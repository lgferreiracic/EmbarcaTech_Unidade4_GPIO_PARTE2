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

// Inicializa as linhas como saída e colunas como entrada
void init_gpio() {
    for (int i = 0; i < ROWS; i++) {
        gpio_init(row_pins[i]);
        gpio_set_dir(row_pins[i], GPIO_OUT);
        gpio_put(row_pins[i], 1); // Linha inicialmente em HIGH
    }

    for (int i = 0; i < COLS; i++) {
        gpio_init(col_pins[i]);
        gpio_set_dir(col_pins[i], GPIO_IN);
        gpio_pull_up(col_pins[i]); // Ativa pull-up nas colunas
    }
}

// Inicializa LEDs e Buzzer
void init_buzzer() {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);
}

// Configura o PWM no pino do buzzer com uma frequência especificada
void set_buzzer_frequency(uint pin, uint frequency) {
    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o pino como saída de PWM
    gpio_set_function(pin, GPIO_FUNC_PWM);

    // Configurar o PWM com frequência desejada
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (frequency * 4096)); // Calcula divisor do clock

    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Inicializa com duty cycle 0 (sem som)
}

// Função para tocar o buzzer por um tempo especificado (em milissegundos)
void play_buzzer(uint pin, uint frequency, uint duration_ms) {

    set_buzzer_frequency(pin, frequency);   
    pwm_set_gpio_level(pin, 32768);           
    sleep_ms(duration_ms);                   
    pwm_set_gpio_level(pin, 0);              
}

// Função para tocar a nota Dó
void playDo(uint duration_ms) {
    play_buzzer(BUZZER_PIN, DO, duration_ms);
}

// Função para tocar a nota Ré
void playRe(uint duration_ms) {
    play_buzzer(BUZZER_PIN, RE, duration_ms);
}

// Função para tocar a nota Mi
void playMi(uint duration_ms){
    play_buzzer(BUZZER_PIN,MI,duration_ms);
}

// Função para tocar a nota Fá
void playFa(uint duration_ms){
    play_buzzer(BUZZER_PIN,FA,duration_ms);
}

// Função para tocar a nota Sol
void playSol(uint duration_ms) {
    play_buzzer(BUZZER_PIN, SOL, duration_ms);
}

// Função para tocar a nota Lá
void playLa(uint duration_ms) {
    play_buzzer(BUZZER_PIN, LA, duration_ms);
}

// Função para tocar a nota Si
void playSi(uint duration_ms) {
    play_buzzer(BUZZER_PIN, SI, duration_ms);
}

// Verifica qual tecla foi pressionada
char scan_keypad() {
    for (int row = 0; row < ROWS; row++) {
        gpio_put(row_pins[row], 0); // Configura a linha atual como LOW

        for (int col = 0; col < COLS; col++) {
            if (gpio_get(col_pins[col]) == 0) { 
                while (gpio_get(col_pins[col]) == 0); 
                gpio_put(row_pins[row], 1); 
                return key_map[row][col];
            }
        }

        gpio_put(row_pins[row], 1); // Restaura a linha para HIGH
    }

    return '\0'; 
}

int main()
{
    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
