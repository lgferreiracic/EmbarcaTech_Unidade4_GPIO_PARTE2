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
// Define o número de animações
#define NUM_ANIMATIONS 10
// Define delay para 24 FPS
#define FRAME_DELAY 8

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
const RGB BLACK = {0, 0, 0};

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

int musics[NUM_ANIMATIONS][NUM_FRAMES] = {
    {DO, MI, SOL, MI, DO},
    {SI, SOL, FA, SOL, SI},
    {MI, SOL, LA, SOL, MI},
    {DO, FA, SI, FA, DO},
    {SI, DO, RE, MI, FA},
    {MI, FA, SOL, LA, SI},
    {DO, RE, MI, FA, SOL},
    {LA, SI, DO, RE, MI},
    {FA, SOL, LA, SI, DO},
    {RE, MI, FA, SOL, LA}
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

//rotina para acionar a matrix de leds - ws2812b
void desenho_pio(RGB pixels[NUM_PIXELS], PIO pio, uint sm) {
    for (int i = 0; i < NUM_PIXELS; i++) {
        pio_sm_put_blocking(pio, sm, matrix_rgb(pixels[i].R, pixels[i].G, pixels[i].B));
    }
}

// Função para reproduzir uma animação
void play_animation(PIO pio, uint sm, int animationIndex) {
    for (int i = 0; i < 5; i++) {
        for (int frame = 0; frame < NUM_FRAMES; ++frame) {
            for (int pixel = 0; pixel < NUM_PIXELS; ++pixel) {
                desenho_pio(animations[animationIndex].frames[frame].pixels, pio, sm);
                sleep_ms(FRAME_DELAY);   
            }
        }
    }
}

// Define os 5 frames para cada animação
void initialize_animations() {
    // Define os padrões de cores para cada animação
    const Animation patterns[NUM_ANIMATIONS] = {
        // Animação 0
        {
            .frames = {
                {
                    .pixels = {
                        BLACK, BLACK, RED, BLACK, BLACK, BLACK, RED, RED, RED, BLACK, RED, RED, RED, RED, RED, RED, RED, RED, RED, RED, BLACK, RED, BLACK, RED, BLACK
                        }
                },
                {
                    .pixels = {
                        BLACK, BLACK, MAGENTA, BLACK, BLACK, BLACK, RED, RED, RED, BLACK, MAGENTA, RED, RED, RED, MAGENTA, MAGENTA, RED, RED, RED, MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA, BLACK
                    }
                },
                {
                    .pixels = { 
                        BLACK, BLACK, MAGENTA, BLACK, BLACK, BLACK, MAGENTA, MAGENTA, MAGENTA, BLACK, MAGENTA, MAGENTA, RED, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA, BLACK
                    }
                },
                {
                    .pixels = { 
                        BLACK, BLACK, MAGENTA, BLACK, BLACK, BLACK, MAGENTA, MAGENTA, MAGENTA, BLACK, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, BLACK, MAGENTA, BLACK, MAGENTA, BLACK
                    }
                },
                {
                    .pixels = { 
                        BLACK, BLACK, RED, BLACK, BLACK, BLACK, RED, RED, RED, BLACK, RED, RED, RED, RED, RED, RED, RED, RED, RED, RED, BLACK, RED, BLACK, RED, BLACK
                    }
                },
            }
        },
        // Animação 1
        {
            .frames = {
                {
                    .pixels = {
                        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, YELLOW, BLACK, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                },
                {
                    .pixels = {
                        BLACK, YELLOW, YELLOW, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, YELLOW, BLACK, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                },
                {
                    .pixels = {
                        BLACK, YELLOW, YELLOW, YELLOW, BLACK, YELLOW, BLACK, BLACK, BLACK, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, YELLOW, BLACK, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                },
                {
                    .pixels = {
                        BLACK, YELLOW, YELLOW, YELLOW, BLACK, YELLOW, YELLOW, YELLOW, YELLOW, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, YELLOW, BLACK, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                },
                {
                    .pixels = {
                        BLACK, WHITE, WHITE, WHITE, BLACK, WHITE, WHITE, WHITE, WHITE, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, WHITE, BLACK, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                }
            }
        },
        // Animação 2
        {
            .frames = {
                {
                    .pixels = {
                        GREEN, BLACK, BLACK, BLACK, GREEN, BLACK, GREEN, GREEN, GREEN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, GREEN, BLACK, GREEN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                },
                {
                    .pixels = {
                        GREEN, BLACK, BLACK, BLACK, GREEN, BLACK, GREEN, GREEN, GREEN, BLACK, BLACK, CYAN, BLACK, BLACK, BLACK, BLACK, GREEN, BLACK, GREEN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                },
                {
                    .pixels = {
                        GREEN, BLACK, BLACK, BLACK, GREEN, BLACK, GREEN, GREEN, CYAN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, GREEN, BLACK, GREEN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                },
                {
                    .pixels = {
                        GREEN, CYAN, BLACK, BLACK, GREEN, BLACK, GREEN, GREEN, GREEN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, GREEN, BLACK, GREEN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                },
                {
                    .pixels = {
                        GREEN, BLACK, BLACK, BLACK, GREEN, BLACK, GREEN, GREEN, GREEN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, GREEN, BLACK, GREEN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                }

            }
        },
        // Animação 3
        {
            .frames = {
                {
                    .pixels = {
                        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLUE, BLUE, BLUE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLUE, BLACK, BLUE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                },
                {
                    .pixels = {
                        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, YELLOW, BLUE, BLUE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLUE, BLACK, BLUE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                },
                {
                    .pixels = {
                        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLUE, YELLOW, BLUE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLUE, BLACK, BLUE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                },
                {
                    .pixels = {
                        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLUE, BLUE, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLUE, BLACK, BLUE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                },
                {
                    .pixels = {
                        BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLUE, BLUE, BLUE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLUE, BLACK, BLUE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
                    }
                }
            }

        },
        // Animação 4
        {
            .frames = {
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                },
                {
                    .pixels = {
                        CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED
                    }
                },
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                },
                {
                    .pixels = {
                        CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED
                    }
                },
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                }
            }
        },
        // Animação 5
        {
            .frames = {
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                },
                {
                    .pixels = {
                        CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED
                    }
                },
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                },
                {
                    .pixels = {
                        CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED
                    }
                },
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                }
            }
        },
        // Animação 6
        {
            .frames = {
                {
                    .pixels = {
                        MAGENTA, WHITE, MAGENTA, WHITE, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, WHITE, MAGENTA, WHITE, MAGENTA, WHITE, MAGENTA, MAGENTA, MAGENTA, WHITE
                    }
                },
                {
                    .pixels = {
                        WHITE, MAGENTA, WHITE, MAGENTA, WHITE, WHITE, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, WHITE, WHITE, MAGENTA, WHITE, MAGENTA, WHITE, MAGENTA, MAGENTA, MAGENTA, WHITE, WHITE
                    }
                },
                {
                    .pixels = {
                        MAGENTA, WHITE, MAGENTA, WHITE, WHITE, WHITE, WHITE, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, WHITE, WHITE, WHITE, WHITE, MAGENTA, WHITE, MAGENTA, MAGENTA, MAGENTA, WHITE, WHITE, WHITE
                    }
                },
                {
                    .pixels = {
                        WHITE, MAGENTA, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, MAGENTA, MAGENTA, MAGENTA, MAGENTA, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, MAGENTA, WHITE, MAGENTA, WHITE, WHITE, WHITE, WHITE
                    }
                },
                {
                    .pixels = {
                        MAGENTA, WHITE, MAGENTA, WHITE, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, MAGENTA, WHITE, MAGENTA, WHITE, MAGENTA, WHITE, MAGENTA, MAGENTA, MAGENTA, WHITE
                    }
                }
            }
        },
        // Animação 7
        {
            .frames = {
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                },
                {
                    .pixels = {
                        CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED
                    }
                },
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                },
                {
                    .pixels = {
                        CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED
                    }
                },
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                }
            }
        },
        // Animação 8
        {
            .frames = {
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                },
                {
                    .pixels = {
                        CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED
                    }
                },
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                },
                {
                    .pixels = {
                        CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED
                    }
                },
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN, CYAN, YELLOW, BLUE, GREEN, RED, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                }
            }
        },
        // Animação 9
        {
            .frames = {
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, RED, GREEN, BLUE, YELLOW, CYAN, RED, GREEN, BLUE, YELLOW, CYAN, RED, GREEN, BLUE, YELLOW, CYAN, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                },
                {
                    .pixels = {
                        CYAN, YELLOW, BLUE, GREEN, RED, CYAN, YELLOW, BLUE, GREEN, RED, CYAN, YELLOW, BLUE, GREEN, RED, CYAN, YELLOW, BLUE, GREEN, RED, CYAN, YELLOW, BLUE, GREEN, RED
                    }
                },
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, RED, GREEN, BLUE, YELLOW, CYAN, RED, GREEN, BLUE, YELLOW, CYAN, RED, GREEN, BLUE, YELLOW, CYAN, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                },
                {
                    .pixels = {
                        CYAN, YELLOW, BLUE, GREEN, RED, CYAN, YELLOW, BLUE, GREEN, RED, CYAN, YELLOW, BLUE, GREEN, RED, CYAN, YELLOW, BLUE, GREEN, RED, CYAN, YELLOW, BLUE, GREEN, RED
                    }
                },
                {
                    .pixels = {
                        RED, GREEN, BLUE, YELLOW, CYAN, RED, GREEN, BLUE, YELLOW, CYAN, RED, GREEN, BLUE, YELLOW, CYAN, RED, GREEN, BLUE, YELLOW, CYAN, RED, GREEN, BLUE, YELLOW, CYAN
                    }
                }
            }
        }
    
    };

    // Copia os padrões para a matriz global de animações
    for (int i = 0; i < NUM_ANIMATIONS; i++) {
        for (int j = 0; j < NUM_FRAMES; j++) {
            animations[i].frames[j] = patterns[i].frames[j];
        }
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
            play_animation(pio, sm, 0);
            break;
        case '1':
            //Faz a animação 1
            play_animation(pio, sm, 1);
            break;
        case '2':
            //Faz a animação 2
            play_animation(pio, sm, 2);
            break;
        case '3':
            //Faz a animação 3
            play_animation(pio, sm, 3);
            break;
        case '4':
            //Faz a animação 4
            play_animation(pio, sm, 4);
            break;
        case '5':
            //Faz a animação 5
            play_animation(pio, sm, 5);
            break;
        case '6':
            //Faz a animação 6
            play_animation(pio, sm, 6);
            break;
        case '7':
            //Faz a animação 7
            play_animation(pio, sm, 7);
            break;
        case '8':
            //Faz a animação 8
            play_animation(pio, sm, 8);
            break;
        case '9':
            //Faz a animação 9
            play_animation(pio, sm, 9);
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

// Função para ler o comando do terminal
void lerComando(char *comando, size_t tamanho) {
    printf("Digite um comando: ");
    memset(comando, 0, tamanho);  // Limpa o buffer do comando
    size_t index = 0;
    while (1) {
        char c = getchar();  // Lê um caractere do terminal
        if (c == '\r' || c == '\n') {
            comando[index] = '\0';
            break;
        } else if (index < tamanho - 1) {
            comando[index++] = c;  // Armazena o caractere no buffer de comando
            putchar(c);  // Mostra o caractere digitado no terminal
        }
    }
    printf("\n");
}

// Função para ler o comando do terminal, do 0 ao 9
void processarComando(const char *comando, PIO pio, uint sm) {
    if(strcmp(comando, "0") == 0){
        for(int i = 0; i<5; i++){
            play_buzzer(BUZZER_PIN, musics[0][i], 200);
        }
        play_animation(pio, sm, 0);
    } else if(strcmp(comando, "1") == 0){
        for(int i = 0; i<5; i++){
            play_buzzer(BUZZER_PIN, musics[1][i], 200);
        }
        play_animation(pio, sm, 1);
    } else if(strcmp(comando, "2") == 0){
        for(int i = 0; i<5; i++){
            play_buzzer(BUZZER_PIN, musics[2][i], 200);
        }
        play_animation(pio, sm, 2);
    } else if(strcmp(comando, "3") == 0){
        for(int i = 0; i<5; i++){
            play_buzzer(BUZZER_PIN, musics[3][i], 200);
        }
        play_animation(pio, sm, 3);
    } else if(strcmp(comando, "4") == 0){
        play_animation(pio, sm, 4);
    } else if(strcmp(comando, "5") == 0){
        play_animation(pio, sm, 5);
    } else if(strcmp(comando, "6") == 0){

        play_animation(pio, sm, 6);
    } else if(strcmp(comando, "7") == 0){
        play_animation(pio, sm, 7);
    } else if(strcmp(comando, "8") == 0){
        play_animation(pio, sm, 8);
    } else if(strcmp(comando, "9") == 0){
        play_animation(pio, sm, 9);
    } else if(strcmp(comando, "*") == 0){
        set_leds(pio, sm, 0.0, 0.0, 0.0);
        printf("HABILITANDO O MODO GRAVAÇÃO");
        reset_usb_boot(0,0); //habilita o modo de gravação do microcontrolador
    } else {
        control_leds_and_buzzer(pio, sm, comando[0]);
    }
}

//função principal
int main()
{
    stdio_init_all();
    //init_gpio();
    init_buzzer();
    initialize_animations();

    //coloca a frequência de clock para 128 MHz, facilitando a divisão pelo clock
    set_sys_clock_khz(128000, false);

    //configurações da PIO
    PIO pio = pio0; 
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    char comando[25];  // Variável para armazenar o comando
    int index = 0; // Índice do buffer de comando

    while (true) {
        /*char key = scan_keypad();
        if (key != '\0') {
            printf("Tecla pressionada: %c\n", key);
            control_leds_and_buzzer(pio, sm, key); // Controla LEDs e buzzer
        }*/
        lerComando(comando, sizeof(comando));
        printf("\nComando Detectado: %s\n", comando);
        processarComando(comando, pio, sm); 
    }
}