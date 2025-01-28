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
#define OUT_PIN 7

// Define os GPIOs para as linhas e colunas do teclado matricial 4x4
#define ROWS 4
#define COLS 4

//número de LEDs
#define NUM_PIXELS 25
// Define o número de frames para a animação
#define NUM_FRAMES 5
// Define delay entre os frames
#define FRAME_DELAY 200

// Definição da estrutura RGB
typedef struct {
    double R;
    double G;
    double B;
} RGB;

//Cores 
const RGB RED = {1, 0, 0};
const RGB GREEN = {0, 1, 0};
const RGB BLUE = {0, 0, 1};
const RGB YELLOW = {1, 1, 0};
const RGB CYAN = {0, 1, 1};
const RGB MAGENTA = {1, 0, 1};
const RGB WHITE = {1, 1, 1};
const RGB BLACK = {0, 0, 0};
const RGB ORANGE = {1, 0.5, 0};

// Mapear GPIOs para linhas e colunas
const uint8_t row_pins[ROWS] = {16, 9, 8, 4};
const uint8_t col_pins[COLS] = {17, 18, 19, 20};

// Mapeamento de teclas
char key_map[16] = { '1', '4', '7', '*',
                     '2', '5', '8', '0',
                     '3', '6', '9', '#',
                     'A', 'B', 'C', 'D' };

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
    for (int i = 0; i < 4; i++)
    {
        gpio_init(col_pins[i]); // inicializa o pino como uma GPIO
        gpio_set_dir(col_pins[i], GPIO_IN); // configura o pino coluna como entrada
        gpio_pull_up(col_pins[i]); // ativação de resistor pull-up que quando precionado vai puxar o pino do estado 1 (HIGH) para o 0 (LOW)

        gpio_init(row_pins[i]); // inicializa o pino como uma GPIO
        gpio_set_dir(row_pins[i], GPIO_OUT); // configura o pino como saida
        gpio_put(row_pins[i], 1); // define o estado logico das linhas como 1 (HIGH)
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

// Verifica qual tecla foi pressionada
char scan_keypad() {
for (int row = 0; row < 4; row++)
    {
        gpio_put(row_pins[row], 0); // define o estado da linha atual como baixo (LOW)

        for (int col = 0; col < 4; col++)
        {
            if (gpio_get(col_pins[col]) == 0) // verifica se a coluna está no estado baixo (LOW)
            {
                while (gpio_get(col_pins[col]) == 0); // espera até que a tecla seja liberada
                gpio_put(row_pins[row], 1); // redefine o estado da linha para alto (HIGH)
                return key_map[row * 4 + col]; // retorna o valor da tecla
            }
        }

        gpio_put(row_pins[row], 1); // redefine a linha para alto (HIGH)
    }

    return '\0'; // retorna o caractere nulo se nenhuma tecla for pressionada
}

//rotina para definição da intensidade de cores do led
uint32_t matrix_rgb(double r, double g, double b)
{
  unsigned char R, G, B;
  R = r * 255;
  G = g * 255;
  B = b * 255;
  return (G << 24) | (R << 16) | (B << 8);
}

//rotina para acionar a matrix de leds - ws2812b
void set_leds(PIO pio, uint sm, double r, double g, double b) {
    uint32_t valor_led;
    for (int16_t i = 0; i < NUM_PIXELS; i++) {
        valor_led = matrix_rgb(r, g, b);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

// Função para converter a posição do matriz para uma posição do vetor.
int getIndex(int x, int y) {
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0) {
        return 24-(y * 5 + x); // Linha par (esquerda para direita).
    } else {
        return 24-(y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

//rotina para acionar a matrix de leds - ws2812b
void desenho_pio(RGB pixels[NUM_PIXELS], PIO pio, uint sm) {
    for (int i = 0; i < NUM_PIXELS; i++) {
        int x = i % 5;
        int y = i / 5;
        int index = getIndex(x, y);
        pio_sm_put_blocking(pio, sm, matrix_rgb(pixels[index].R, pixels[index].G, pixels[index].B));
    }
}

// Função para reproduzir a animação 0, que é um coração pulsante
void play_animation_0(PIO pio, uint sm) {
    
    RGB frame1[NUM_PIXELS] = {
        BLACK, RED, BLACK, RED, BLACK,
        RED, RED, RED, RED, RED,
        RED, RED, RED, RED, RED,
        BLACK, RED, RED, RED, BLACK,
        BLACK, BLACK, RED, BLACK, BLACK
    };

    RGB frame2[NUM_PIXELS] = {
        BLACK, MAGENTA, BLACK, MAGENTA, BLACK,
        MAGENTA, RED, RED, RED, MAGENTA,
        MAGENTA, RED, RED, RED, MAGENTA,
        BLACK, RED, RED, RED, BLACK,
        BLACK, BLACK, MAGENTA, BLACK, BLACK
    };

    RGB frame3[NUM_PIXELS] = {
        BLACK, MAGENTA, BLACK, MAGENTA, BLACK,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        MAGENTA, MAGENTA, RED, MAGENTA, MAGENTA,
        BLACK, MAGENTA, MAGENTA, MAGENTA, BLACK,
        BLACK, BLACK, MAGENTA, BLACK, BLACK
    };
       
    RGB frame4[NUM_PIXELS] = {
        BLACK, MAGENTA, BLACK, MAGENTA, BLACK,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        BLACK, MAGENTA, MAGENTA, MAGENTA, BLACK,
        BLACK, BLACK, MAGENTA, BLACK, BLACK
    };

    RGB frame5[NUM_PIXELS] = {
        BLACK, RED, BLACK, RED, BLACK,
        RED, RED, RED, RED, RED,
        RED, RED, RED, RED, RED,
        BLACK, RED, RED, RED, BLACK,
        BLACK, BLACK, RED, BLACK, BLACK
    };
    
    RGB *frames[NUM_FRAMES] = {frame1, frame2, frame3, frame4, frame5};
    int notas[5] = {DO, MI, SOL, MI, DO};
    for (int i = 0; i < 5; i++) {
        play_buzzer(BUZZER_PIN, notas[i], 200);
    }

    for(int i = 0; i < 5; i++){
        for (int i = 0; i < NUM_FRAMES; i++) {
            desenho_pio(frames[i], pio, sm);
            sleep_ms(FRAME_DELAY);
        }
    }
}

// Função para reproduzir a animação 1, que é um rosto feliz
void play_animation_1(PIO pio, uint sm) {
    RGB frame1[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, YELLOW, BLACK, YELLOW, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame2[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, YELLOW, BLACK, YELLOW, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, YELLOW, YELLOW, YELLOW, BLACK
    };

    RGB frame3[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, YELLOW, BLACK, YELLOW, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        YELLOW, BLACK, BLACK, BLACK, YELLOW,
        BLACK, YELLOW, YELLOW, YELLOW, BLACK
    };

    RGB frame4[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, YELLOW, BLACK, YELLOW, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        YELLOW, YELLOW, YELLOW, YELLOW, YELLOW,
        BLACK, YELLOW, YELLOW, YELLOW, BLACK
    };

    RGB frame5[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, ORANGE, BLACK, ORANGE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        ORANGE, ORANGE, ORANGE, ORANGE, ORANGE,
        BLACK, ORANGE, ORANGE, ORANGE, BLACK
    };

    RGB frame6[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, ORANGE, BLACK, ORANGE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        ORANGE, BLACK, BLACK, BLACK, ORANGE,
        BLACK, ORANGE, ORANGE, ORANGE, BLACK
    };

    RGB frame7[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, ORANGE, BLACK, ORANGE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, ORANGE, ORANGE, ORANGE, BLACK
    };

    RGB frame8[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, ORANGE, BLACK, ORANGE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB *frames[8] = {frame1, frame2, frame3, frame4, frame5, frame6, frame7, frame8};
    int notas[5] = {SI, SOL, FA, SOL, SI};
    for (int i = 0; i < 5; i++) {
        play_buzzer(BUZZER_PIN, notas[i], 200);
    }

    for (int i = 0; i < 3; i++) {
        for (int i = 0; i < 8; i++) {
            desenho_pio(frames[i], pio, sm);
            sleep_ms(FRAME_DELAY);
        }
    }
}

// Função para reproduzir a animação 2, que é um rosto triste
void play_animation_2(PIO pio, uint sm) {
    RGB frame1[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, GREEN, BLACK, GREEN, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, GREEN, GREEN, GREEN, BLACK,
        GREEN, BLACK, BLACK, BLACK, GREEN
    };

    RGB frame2[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, GREEN, BLACK, GREEN, BLACK,
        BLACK, BLACK, BLACK, CYAN, BLACK,
        BLACK, GREEN, GREEN, GREEN, BLACK,
        GREEN, BLACK, BLACK, BLACK, GREEN
    };

    RGB frame3[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, GREEN, BLACK, GREEN, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, GREEN, GREEN, CYAN, BLACK,
        GREEN, BLACK, BLACK, BLACK, GREEN
    };

    RGB frame4[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, GREEN, BLACK, GREEN, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, GREEN, GREEN, GREEN, BLACK,
        GREEN, BLACK, BLACK, CYAN, GREEN
    };

    RGB frame5[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, GREEN, BLACK, GREEN, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, GREEN, GREEN, GREEN, BLACK,
        GREEN, BLACK, BLACK, BLACK, GREEN
    };
    
    RGB *frames[NUM_FRAMES] = {frame1, frame2, frame3, frame4, frame5};
    int notas[5] = {MI, SOL, LA, SOL, MI};
    for (int i = 0; i < 5; i++) {
        play_buzzer(BUZZER_PIN, notas[i], 200);
    }
    for (int i = 0; i < 5; i++) {
        for (int i = 0; i < NUM_FRAMES; i++) {
            desenho_pio(frames[i], pio, sm);
            sleep_ms(FRAME_DELAY);
        }
    }
}

/// Função para reproduzir a animação 3, que é um robô
void play_animation_3(PIO pio, uint sm) {
    RGB frame1[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame2[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, YELLOW, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame3[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, YELLOW, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame4[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLUE, YELLOW, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame5[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, RED, BLACK, RED, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, RED, RED, RED, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB *frames[4] = {frame1, frame2, frame3, frame4};
    int notas[5] = {DO, FA, SI, FA, DO};
    for (int i = 0; i < 5; i++) {
        play_buzzer(BUZZER_PIN, notas[i], 200);
    }
    for (int i = 0; i < 5; i++) {
        for (int i = 0; i < 4; i++) {
            desenho_pio(frames[i], pio, sm);
            sleep_ms(FRAME_DELAY);
        }
    }
    desenho_pio(frame5, pio, sm);
    play_buzzer(BUZZER_PIN, DO, 200);
}

// Função para reproduzir a animação 4, que é um quadrado
void play_animation_4(PIO pio, uint sm) {

    RGB frame1[NUM_PIXELS] = {
        YELLOW, YELLOW, YELLOW, YELLOW, YELLOW,
        YELLOW, BLACK, BLACK, BLACK, YELLOW,
        YELLOW, BLACK, BLACK, BLACK, YELLOW,
        YELLOW, BLACK, BLACK, BLACK, YELLOW,
        YELLOW, YELLOW, YELLOW, YELLOW, YELLOW
    };

    RGB frame2[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, YELLOW, YELLOW, YELLOW, BLACK,
        BLACK, YELLOW, BLACK, YELLOW, BLACK,
        BLACK, YELLOW, YELLOW, YELLOW, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame3[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, YELLOW, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame4[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, YELLOW, YELLOW, YELLOW, BLACK,
        BLACK, YELLOW, BLACK, YELLOW, BLACK,
        BLACK, YELLOW, YELLOW, YELLOW, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame5[NUM_PIXELS] = {
        YELLOW, YELLOW, YELLOW, YELLOW, YELLOW,
        YELLOW, BLACK, BLACK, BLACK, YELLOW,
        YELLOW, BLACK, BLACK, BLACK, YELLOW,
        YELLOW, BLACK, BLACK, BLACK, YELLOW,
        YELLOW, YELLOW, YELLOW, YELLOW, YELLOW
    };

    RGB *frames[NUM_FRAMES] = {frame1, frame2, frame3, frame4, frame5};
    for (int i = 0; i < NUM_FRAMES; i++) {
        for (int i = 0; i < NUM_FRAMES; i++) {
            desenho_pio(frames[i], pio, sm);
            sleep_ms(FRAME_DELAY);
    }
    }
} 

// Função para reproduzir a animação 5, que é um relógio
void play_animation_5(PIO pio, uint sm) {
    RGB frame1[NUM_PIXELS] = {
        BLACK, BLACK, CYAN, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame2[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, CYAN,
        BLACK, BLACK, BLUE, CYAN, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame3[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLUE, CYAN, CYAN,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame4[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, CYAN, BLACK,
        BLACK, BLACK, BLACK, BLACK, CYAN
    };

    RGB frame5[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, CYAN, BLACK, BLACK,
        BLACK, BLACK, CYAN, BLACK, BLACK
    };

    RGB frame6[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, CYAN, BLACK, BLACK, BLACK,
        CYAN, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame7[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        CYAN, CYAN, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame8[NUM_PIXELS] = {
        CYAN, BLACK, BLACK, BLACK, BLACK,
        BLACK, CYAN, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame9[NUM_PIXELS] = {
        BLACK, BLACK, CYAN, BLACK, BLACK,
        BLACK, BLACK, CYAN, BLUE, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame10[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, CYAN,
        BLACK, BLACK, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame11[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLUE, CYAN, CYAN,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame12[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, CYAN, BLACK,
        BLACK, BLACK, BLACK, BLACK, CYAN
    };

    RGB frame13[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, CYAN, BLACK, BLACK,
        BLACK, BLACK, CYAN, BLACK, BLACK
    };

    RGB frame14[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, CYAN, BLACK, BLACK, BLACK,
        CYAN, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame15[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLUE, BLACK,
        CYAN, CYAN, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame16[NUM_PIXELS] = {
        CYAN, BLACK, BLACK, BLACK, BLACK,
        BLACK, CYAN, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame17[NUM_PIXELS] = {
        BLACK, BLACK, CYAN, BLACK, BLACK,
        BLACK, BLACK, CYAN, BLUE, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame18[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, CYAN,
        BLACK, BLACK, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame19[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, CYAN,
        BLACK, BLACK, BLACK, CYAN, BLACK,
        BLACK, BLACK, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame20[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLUE, CYAN,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame21[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, CYAN, BLACK,
        BLACK, BLACK, BLACK, BLACK, CYAN
    };

    RGB frame22[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLUE, BLACK,
        BLACK, BLACK, CYAN, BLACK, BLACK,
        BLACK, BLACK, CYAN, BLACK, BLACK
    };

    RGB frame23[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLUE, BLACK,
        BLACK, CYAN, BLACK, BLACK, BLACK,
        CYAN, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame24[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        CYAN, CYAN, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame25[NUM_PIXELS] = {
        CYAN, BLACK, BLACK, BLACK, BLACK,
        BLACK, CYAN, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame26[NUM_PIXELS] = {
        BLACK, BLACK, CYAN, BLACK, BLACK,
        BLACK, BLACK, CYAN, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame27[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, CYAN,
        BLACK, BLACK, BLACK, CYAN, BLACK,
        BLACK, BLACK, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame28[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLUE, CYAN,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame29[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, CYAN, CYAN,
        BLACK, BLACK, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB *frames[30] = {frame1, frame2, frame3, frame4, frame5, frame6, frame7, frame8, frame1, 
                        frame9, frame10, frame11, frame12, frame13, frame14, frame15, frame16, 
                        frame17, frame18, frame19, frame20, frame21, frame22, frame23, frame24, 
                        frame25, frame26, frame27, frame28, frame29};

    for (int i = 0; i < 30; i++) {
        desenho_pio(frames[i], pio, sm);
        play_buzzer(BUZZER_PIN, MI, 100);
        sleep_ms(FRAME_DELAY);
    }
    
}

// Função para reproduzir a animação 6, que é o fantasma do Pac Man
void play_animation_6(PIO pio, uint sm) {
    RGB frame1[NUM_PIXELS] = {
        BLACK, MAGENTA, MAGENTA, MAGENTA, BLACK,
        MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA
    };

    RGB frame2[NUM_PIXELS] = {
        BLACK, BLACK, MAGENTA, MAGENTA, MAGENTA,
        BLACK, MAGENTA, BLACK, MAGENTA, BLACK,
        BLACK, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        BLACK, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        BLACK, MAGENTA, BLACK, MAGENTA, BLACK
    };

    RGB frame3[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, MAGENTA, MAGENTA,
        BLACK, BLACK, MAGENTA, BLACK, MAGENTA, 
        BLACK, BLACK, MAGENTA, MAGENTA, MAGENTA, 
        BLACK, BLACK, MAGENTA, MAGENTA, MAGENTA, 
        BLACK, BLACK, MAGENTA, BLACK, MAGENTA, 
    };

    RGB frame4[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, MAGENTA,
        BLACK, BLACK, BLACK, MAGENTA, BLACK,
        BLACK, BLACK, BLACK, MAGENTA, MAGENTA,
        BLACK, BLACK, BLACK, MAGENTA, MAGENTA,
        BLACK, BLACK, BLACK, MAGENTA, BLACK
    };

    RGB frame5[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, MAGENTA,
        BLACK, BLACK, BLACK, BLACK, MAGENTA,
        BLACK, BLACK, BLACK, BLACK, MAGENTA,
        BLACK, BLACK, BLACK, BLACK, MAGENTA,
    };

    RGB frame6[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
    };

    RGB frame7[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        MAGENTA, BLACK, BLACK, BLACK, BLACK,
        MAGENTA, BLACK, BLACK, BLACK, BLACK,
        MAGENTA, BLACK, BLACK, BLACK, BLACK,
        MAGENTA, BLACK, BLACK, BLACK, BLACK,
    };

    RGB frame8[NUM_PIXELS] = {
        MAGENTA, BLACK, BLACK, BLACK, BLACK,
        BLACK, MAGENTA, BLACK, BLACK, BLACK,
        MAGENTA, MAGENTA, BLACK, BLACK, BLACK,
        MAGENTA, MAGENTA, BLACK, BLACK, BLACK,
        BLACK, MAGENTA, BLACK, BLACK, BLACK,
    };

    RGB frame9[NUM_PIXELS] = {
        MAGENTA, MAGENTA, BLACK, BLACK, BLACK,
        MAGENTA, BLACK, MAGENTA, BLACK, BLACK,
        MAGENTA, MAGENTA, MAGENTA, BLACK, BLACK,
        MAGENTA, MAGENTA, MAGENTA, BLACK, BLACK,
        MAGENTA, BLACK, MAGENTA, BLACK, BLACK,
    };

    RGB frame10[NUM_PIXELS] = {
        MAGENTA, MAGENTA, MAGENTA, BLACK, BLACK,
        BLACK, MAGENTA, BLACK, MAGENTA, BLACK,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, BLACK,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, BLACK,
        BLACK, MAGENTA, BLACK, MAGENTA, BLACK,
    };

    RGB frame11[NUM_PIXELS] = {
        BLACK, MAGENTA, MAGENTA, MAGENTA, BLACK,
        MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA
    };

    RGB frame12[NUM_PIXELS] = {
        BLACK, MAGENTA, MAGENTA, MAGENTA, BLACK,
        MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        BLACK, MAGENTA, BLACK, MAGENTA, BLACK
    };

    RGB frame13[NUM_PIXELS] = {
        BLACK, MAGENTA, MAGENTA, MAGENTA, BLACK,
        MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA
    };

    RGB frame14[NUM_PIXELS] = {
        BLACK, MAGENTA, MAGENTA, MAGENTA, BLACK,
        MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        BLACK, MAGENTA, BLACK, MAGENTA, BLACK
    };

    RGB frame15[NUM_PIXELS] = {
        BLACK, MAGENTA, MAGENTA, MAGENTA, BLACK,
        MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA,
        MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA
    };

    RGB *frames[15] = {frame1, frame2, frame3, frame4, frame5, frame6, frame7, frame8, frame9, frame10, frame11, frame12, frame13, frame14, frame15};
    for (int i = 0; i < 2; i++) {
            for (int i = 0; i < 15; i++) {
            desenho_pio(frames[i], pio, sm);
            sleep_ms(FRAME_DELAY);
        }
    }
}

// Função para reproduzir a animação 7, que é um cacto
void play_animation_7(PIO pio, uint sm) {
    RGB frame1[NUM_PIXELS] = {
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        GREEN, GREEN, GREEN, GREEN, GREEN,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
    };

    RGB frame2[NUM_PIXELS] = {
        BLACK, BLACK, GREEN, BLACK, BLACK,
        GREEN, BLACK, GREEN, BLACK, GREEN,
        GREEN, GREEN, GREEN, GREEN, GREEN,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
    };

    RGB frame3[NUM_PIXELS] = {
        GREEN, BLACK, GREEN, BLACK, GREEN,
        GREEN, BLACK, GREEN, BLACK, GREEN,
        GREEN, GREEN, GREEN, GREEN, GREEN,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
    };

    RGB frame4[NUM_PIXELS] = {
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        GREEN, GREEN, GREEN, GREEN, GREEN,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
    };

    RGB frame5[NUM_PIXELS] = {
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, GREEN, GREEN, GREEN, WHITE,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
    };

    RGB frame6[NUM_PIXELS] = {
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
    };

    RGB frame7[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
    };

    RGB frame8[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, GREEN, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
    };

    RGB *frames[8] = {frame1, frame2, frame3, frame4, frame5,frame6,frame7,frame8};
    for (int i = 0; i < 2; i++) {
        for (int i = 0; i < 8; i++) {
            desenho_pio(frames[i], pio, sm);
            sleep_ms(FRAME_DELAY);
        }
        
    }
}

// Função para reproduzir a animação 8 (Chamas)
void play_animation_8(PIO pio, uint sm) {
    
    RGB frame1[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, RED, BLACK, RED, BLACK,
        RED, ORANGE, RED, ORANGE, RED,
        RED, ORANGE, YELLOW, ORANGE, RED,
        BLACK, ORANGE, YELLOW, ORANGE, BLACK
    };

    RGB frame2[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        RED, BLACK, RED, BLACK, RED,
        RED, YELLOW, ORANGE, YELLOW, RED,
        RED, ORANGE, ORANGE, ORANGE, RED,
        ORANGE, RED, YELLOW, RED, ORANGE
    };

    RGB frame3[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, RED, BLACK, RED, BLACK,
        ORANGE, RED, YELLOW, RED, ORANGE,
        RED, ORANGE, ORANGE, ORANGE, RED,
        RED, ORANGE, YELLOW, ORANGE, BLACK
    };

    RGB frame4[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, RED, BLACK, YELLOW, BLACK,
        RED, ORANGE, ORANGE, ORANGE, RED,
        ORANGE, RED, YELLOW, RED, ORANGE,
        BLACK, ORANGE, YELLOW, ORANGE, BLACK
    };

    RGB frame5[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, RED, BLACK, BLACK,
        BLACK, RED, ORANGE, RED, BLACK,
        ORANGE, YELLOW, ORANGE, YELLOW, ORANGE,
        BLACK, RED, ORANGE, RED, BLACK
    };

    
    RGB *frames[NUM_FRAMES] = {frame1, frame2, frame3, frame4, frame5};

    // Frequências para os sons de "crepitar"
    int frequencies[NUM_FRAMES] = {
        MI, FA, SOL, LA, MI // Tons para cada frame
    };

    int cont = 0;
    while (cont < 3) {
        for (int i = 0; i < NUM_FRAMES; i++) {
            desenho_pio(frames[i], pio, sm);         
            play_buzzer(BUZZER_PIN, frequencies[i], 100);
            sleep_ms(FRAME_DELAY);                 
        }
        cont++;
    }
}


// Função para reproduzir a animação 9 (Espiral)
void play_animation_9(PIO pio, uint sm) {

    // Frequências para os sons (uma para cada LED da espiral)
int frequencies[25] = {
    DO, RE, MI, FA, SOL, LA, SI, DO * 2, RE * 2, MI * 2,
    FA * 2, SOL * 2, LA * 2, SI * 2, DO * 3, RE * 3, MI * 3,
    FA * 3, SOL * 3, LA * 3, SI * 3, DO * 4, RE * 4, MI * 4, FA * 4
};



    RGB frame1[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame2[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame3[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLUE, BLACK,
        BLACK, BLACK, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame4[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame5[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame6[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame7[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLUE, BLACK, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame8[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLUE, BLUE, BLACK, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame9[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame10[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame11[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame12[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLACK,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame13[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLACK, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame14[NUM_PIXELS] = {
        BLACK, BLACK, BLACK, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame15[NUM_PIXELS] = {
        BLACK, BLACK, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame16[NUM_PIXELS] = {
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame17[NUM_PIXELS] = {
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame18[NUM_PIXELS] = {
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame19[NUM_PIXELS] = {
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame20[NUM_PIXELS] = {
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLACK, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame21[NUM_PIXELS] = {
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLACK, BLACK, BLACK, BLACK
    };

    RGB frame22[NUM_PIXELS] = {
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLACK, BLACK, BLACK
    };

    RGB frame23[NUM_PIXELS] = {
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLACK, BLACK
    };

    RGB frame24[NUM_PIXELS] = {
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLACK
    };

    RGB frame25[NUM_PIXELS] = {
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE,
        BLUE, BLUE, BLUE, BLUE, BLUE
    };

    
    RGB *frames[25] = {frame1, frame2, frame3, frame4, frame5, frame6, frame7, frame8,
                       frame9, frame10, frame11, frame12, frame13, frame14, frame15, frame16,
                       frame17, frame18, frame19, frame20, frame21, frame22, frame23, frame24,
                       frame25};


    for (int i = 0; i < 25; i++) {
        desenho_pio(frames[i], pio, sm); 
        play_buzzer(BUZZER_PIN, frequencies[i], 100); // Toca o som correspondente
        sleep_ms(80); // Ajuste de delay
    }
}

// Função para controlar LEDs e Buzzer com base na tecla pressionada
void control_leds_and_buzzer(PIO pio, uint sm, char key) {
    switch (key) {
        case 'A':
            set_leds(pio, sm, 0.0, 0.0, 0.0); // Desliga todos os LEDs
            break;
        case 'B':
            set_leds(pio, sm, 0.0, 0.0, 1.0); // LEDs azul com 100% de intensidade
            break;
        case 'C':
            set_leds(pio, sm, 0.8, 0.0, 0.0); // LEDs vermelho com 80% de intensidade
            break;
        case 'D':
            set_leds(pio, sm, 0.0, 0.5, 0.0); // LEDs verde com 50% de intensidade
            break;
        case '#':
            set_leds(pio, sm, 0.2, 0.2, 0.2); // LEDs branco com 20% de intensidade
            break;
        case '0': 
            //Faz a animação 0
            play_animation_0(pio, sm);
            break;
        case '1':
            //Faz a animação 1
            play_animation_1(pio, sm);
            break;
        case '2':
            //Faz a animação 2
            play_animation_2(pio, sm);
            break;
        case '3':
            //Faz a animação 3
            play_animation_3(pio, sm);
            break;
        case '4':
            //Faz a animação 4
            play_animation_4(pio, sm);
            break;
        case '5':
            //Faz a animação 5
            play_animation_5(pio, sm);
            break;
        case '6':
            //Faz a animação 6
            play_animation_6(pio, sm);
            break;
        case '7':
            //Faz a animação 7
            play_animation_7(pio, sm);
            break;
        case '8':
            //Faz a animação 8
            play_animation_8(pio, sm);
            break;
        case '9':
            //Faz a animação 9
            play_animation_9(pio, sm);
            break;
        case '*':
            set_leds(pio, sm, 0.0, 0.0, 0.0);
            printf("HABILITANDO O MODO GRAVAÇÃO");
	        reset_usb_boot(0,0); //habilita o modo de gravação do microcontrolador
            break;
        default:
            break;
    }
}

//função principal
int main()
{
    stdio_init_all();
    init_gpio();
    init_buzzer();

    //configurações da PIO
    PIO pio = pio0; 
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    while (true) {
        char key = scan_keypad();
        if (key != '\0') {
            printf("Tecla pressionada: %c\n", key);
            control_leds_and_buzzer(pio, sm, key); // Controla LEDs e buzzer
        }
    }
}