#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "hardware/pwm.h"

//arquivo .pio
#include "EmbarcaTech_Unidade4_GPIO_PARTE2.pio.h"

//Definição da porta GGPIO do BUZZER
#define BUZZER_PIN 21

//pino de saída
#define OUT_PIN 11

// Define os GPIOs para as linhas e colunas do teclado matricial 4x4
#define ROWS 4
#define COLS 4

//número de LEDs
#define NUM_PIXELS 25
// Define o número de frames para a animação
#define NUM_FRAMES 10
// Define o número de animações
#define NUM_ANIMATIONS 10
// Define delay para 60 FPS
#define FRAME_DELAY 16

// Definição da estrutura RGB
typedef struct {
    double R;
    double G;
    double B;
} RGB;

// Define estrutura para um frame
typedef struct {
    RGB pixels[NUM_PIXELS];
} Frame;

// Define estrutura para uma animação
typedef struct {
    Frame frames[NUM_FRAMES];
} Animation;

// Declaração do array para armazenar as animações
Animation animations[NUM_ANIMATIONS];

//Cores 
const RGB RED = {1, 0, 0};
const RGB GREEN = {0, 1, 0};
const RGB BLUE = {0, 0, 1};
const RGB YELLOW = {1, 1, 0};
const RGB CYAN = {0, 1, 1};
const RGB MAGENTA = {1, 0, 1};
const RGB WHITE = {1, 1, 1};

// Mapear GPIOs para linhas e colunas
const uint8_t row_pins[ROWS] = {8, 7, 6, 5};
const uint8_t col_pins[COLS] = {4, 3, 2, 1};

// Matriz de teclas
const char key_map[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// Frequências das notas musicais (em Hz)
enum NotasMusicais {
    DO = 2640, // Dó
    RE = 2970, // Ré
    MI = 3300, // Mi
    FA = 3520, // Fá
    SOL = 3960, // Sol
    LA = 4400, // Lá
    SI = 4950  // Si
};

int main()
{
    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
