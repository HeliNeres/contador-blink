#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/bootrom.h"

//arquivo .pio
#include "matriz.pio.h"

int digitos[10][25] = {
    {0,50,50,50,0,
     50,50,0,0,50,
     50,0,500,0,50,
     50,0,0,50,50,
     0,50,50,50,0},
    {0,0,50,0,0,
     0,0,50,50,0,
     0,0,50,0,0,
     0,0,50,0,0,
     0,50,50,50,0},
    {50,50,50,50,0,
     50,0,0,0,0,
     0,50,50,50,0,
     0,0,0,0,50,
     50,50,50,50,50},
    {50,50,50,50,0,
     50,0,0,0,0,
     0,0,50,50,0,
     50,0,0,0,0,
     50,50,50,50,0},
    {0,0,50,50,0,
     0,50,0,50,0,
     50,0,0,50,0,
     50,50,50,50,50,
     0,0,0,50,0},
    {50,50,50,50,50,
     0,0,0,0,50,
     50,50,50,50,0,
     50,0,0,0,0,
     50,50,50,50,0},
    {0,50,50,50,50,
     0,0,0,0,50,
     50,50,50,50,0,
     50,0,0,0,50,
     0,50,50,50,0},
    {50,50,50,50,50,
     50,0,0,0,0,
     0,0,0,50,0,
     0,0,50,0,0,
     0,50,0,0,0},
    {0,50,50,50,0,
     50,0,0,0,50,
     0,50,50,50,0,
     50,0,0,0,50,
     0,50,50,50,0},
    {0,50,50,50,0,
     50,0,0,0,50,
     0,50,50,50,50,
     50,0,0,0,0,
     50,50,50,50,0},
};

//número de LEDs
#define NUM_LEDS 25

//pino de saída
#define OUT_PIN 7
#define LED_PIN 13

//botões da BitDogLab
const uint button_0 = 5;
const uint button_1 = 6;
uint last_time = 0;
uint8_t last_pin = 0;
int8_t num = 0;

PIO pio;
uint sm;

//rotina para acionar a matrix de leds - ws2812b
void desenho_pio(int desenho[25], PIO pio, uint sm){
    for (int16_t i = 0; i < NUM_LEDS; i++) {
        pio_sm_put_blocking(pio, sm, desenho[24-i]);
        pio_sm_put_blocking(pio, sm, 0);
        pio_sm_put_blocking(pio, sm, 0);
    }
}

//rotina da interrupção
static void gpio_irq_handler(uint gpio, uint32_t events){
    uint current_time = to_us_since_boot(get_absolute_time());
    uint8_t current_pin = gpio;
    //printf("Interrupção ocorreu no pino %d, no evento %d, número %d\n", gpio, events, num);
    if ((current_time - last_time > 200000) && (current_pin == last_pin)){ //debouncing de 100ms
        if (gpio == 5) num--;
        else if (gpio == 6) num++;

        if (num > 9) num = 0;
        else if (num < 0) num = 9;

        desenho_pio(digitos[num], pio, sm);
    }
    last_pin = gpio;
}

int main(){
    pio = pio0;
    bool ok;
    uint16_t i;

    //coloca a frequência de clock para 128 MHz, facilitando a divisão pelo clock
    ok = set_sys_clock_khz(128000, false);

    // Inicializa todos os códigos stdio padrão que estão ligados ao binário.
    stdio_init_all();

    printf("iniciando a transmissão PIO");
    if (ok) printf("clock set to %ld\n", clock_get_hz(clk_sys));

    //configurações da PIO
    uint offset = pio_add_program(pio, &matriz_program);
    sm = pio_claim_unused_sm(pio, true);
    matriz_program_init(pio, sm, offset, OUT_PIN, 800000);

    sleep_ms(1000);

    //inicializar o botão de interrupção - GPIO5
    gpio_init(button_0);
    gpio_set_dir(button_0, GPIO_IN);
    gpio_pull_up(button_0);

    //inicializar o botão de interrupção - GPIO5
    gpio_init(button_1);
    gpio_set_dir(button_1, GPIO_IN);
    gpio_pull_up(button_1);

    // inicializar led
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    //interrupção da gpio habilitada
    gpio_set_irq_enabled_with_callback(button_0, GPIO_IRQ_EDGE_FALL, 1, & gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_1, GPIO_IRQ_EDGE_FALL, 1, & gpio_irq_handler);

    desenho_pio(digitos[num], pio, sm);

    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
}

