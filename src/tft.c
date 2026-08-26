#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <math.h>
#include <linux/spi/spidev.h>
#include "tft.h"
#include "data.h"

#define DC_PIN 24
#define RST_PIN 25
#define LED_PIN 27
#define BUTTON_PIN 3

#define BLOCK_SIZE (4*1024)

static int spi_fd = -1;
static volatile uint32_t *tft_gpio_map = MAP_FAILED;

#define INP_GPIO(g) *(tft_gpio_map+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(tft_gpio_map+((g)/10)) |=  (1<<(((g)%10)*3))

#define GPIO_SET *(tft_gpio_map+7)
#define GPIO_CLR *(tft_gpio_map+10)

uint16_t tft_buffer[TFT_BUFFER_SIZE];

static int tft_gpio_init(void) {
    int mem_fd;
    if ((mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
        perror("can't open /dev/gpiomem");
        return -1;
    }
    tft_gpio_map = (uint32_t *)mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, 0);
    close(mem_fd);

    if (tft_gpio_map == MAP_FAILED) {
        perror("mmap error");
        return -1;
    }
    
    INP_GPIO(DC_PIN); OUT_GPIO(DC_PIN);
    INP_GPIO(RST_PIN); OUT_GPIO(RST_PIN);
    INP_GPIO(LED_PIN); OUT_GPIO(LED_PIN);
    
    // Set BUTTON_PIN as input (Hardware pull-up to 3.3V on GPIO 3)
    INP_GPIO(BUTTON_PIN);
    return 0;
}

static void dc_high(void) {
    if (tft_gpio_map != MAP_FAILED) GPIO_SET = 1 << DC_PIN;
}

static void dc_low(void) {
    if (tft_gpio_map != MAP_FAILED) GPIO_CLR = 1 << DC_PIN;
}

static void rst_high(void) {
    if (tft_gpio_map != MAP_FAILED) GPIO_SET = 1 << RST_PIN;
}

static void rst_low(void) {
    if (tft_gpio_map != MAP_FAILED) GPIO_CLR = 1 << RST_PIN;
}

static void spi_write(uint8_t* data, size_t len) {
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)data,
        .rx_buf = 0,
        .len = len,
        .delay_usecs = 0,
        .speed_hz = 15000000, // 15 MHz
        .bits_per_word = 8,
    };
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
}

static void tft_command(uint8_t cmd) {
    dc_low();
    spi_write(&cmd, 1);
}

static void tft_data(uint8_t data) {
    dc_high();
    spi_write(&data, 1);
}

void tft_init(void) {
    if (tft_gpio_init() < 0) {
        printf("TFT GPIO Init failed.\n");
    }
    
    // Hardware reset
    rst_high();
    if (tft_gpio_map != MAP_FAILED) GPIO_SET = 1 << LED_PIN;
    usleep(50000);
    rst_low();
    usleep(50000);
    rst_high();
    usleep(100000);
    
    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd < 0) {
        perror("Failed to open SPI device");
        return;
    }
    
    uint8_t mode = 0;
    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    uint32_t speed = 15000000;
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    // Robust ST7735R Init sequence (Initializes power controllers, charge pump and gamma)
    tft_command(0x01); // SWRESET
    usleep(150000);
    
    tft_command(0x11); // SLPOUT
    usleep(255000);
    
    tft_command(0xB1); // FRMCTR1 (Frame rate control normal mode)
    tft_data(0x01); tft_data(0x2C); tft_data(0x2D);
    
    tft_command(0xB2); // FRMCTR2 (Frame rate control idle mode)
    tft_data(0x01); tft_data(0x2C); tft_data(0x2D);
    
    tft_command(0xB3); // FRMCTR3 (Frame rate control partial mode)
    tft_data(0x01); tft_data(0x2C); tft_data(0x2D);
    tft_data(0x01); tft_data(0x2C); tft_data(0x2D);
    
    tft_command(0xB4); // INVCTR (Display inversion control)
    tft_data(0x07);
    
    tft_command(0xC0); // PWCTR1 (Power control 1)
    tft_data(0xA2); tft_data(0x02); tft_data(0x84);
    
    tft_command(0xC1); // PWCTR2 (Power control 2)
    tft_data(0xC5);
    
    tft_command(0xC2); // PWCTR3 (Power control 3)
    tft_data(0x0A); tft_data(0x00);
    
    tft_command(0xC3); // PWCTR4 (Power control 4)
    tft_data(0x8A); tft_data(0x2A);
    
    tft_command(0xC4); // PWCTR5 (Power control 5)
    tft_data(0x8A); tft_data(0xEE);
    
    tft_command(0xC5); // VMCTR1 (VCOM control 1)
    tft_data(0x0E);
    
    tft_command(0x20); // INVOFF
    
    tft_command(0x36); // MADCTL (Orientation)
    tft_data(0x60);    // Landscape
    
    tft_command(0x3A); // COLMOD (16-bit RGB565)
    tft_data(0x05);
    
    tft_command(0xE0); // GMCTRP1 (Gamma positive)
    tft_data(0x02); tft_data(0x1C); tft_data(0x07); tft_data(0x12);
    tft_data(0x37); tft_data(0x32); tft_data(0x29); tft_data(0x2D);
    tft_data(0x29); tft_data(0x25); tft_data(0x2B); tft_data(0x39);
    tft_data(0x00); tft_data(0x01); tft_data(0x03); tft_data(0x10);
    
    tft_command(0xE1); // GMCTRN1 (Gamma negative)
    tft_data(0x03); tft_data(0x1D); tft_data(0x07); tft_data(0x06);
    tft_data(0x2E); tft_data(0x2C); tft_data(0x29); tft_data(0x2D);
    tft_data(0x2E); tft_data(0x2E); tft_data(0x37); tft_data(0x3F);
    tft_data(0x00); tft_data(0x00); tft_data(0x02); tft_data(0x10);
    
    tft_command(0x13); // NORON
    usleep(10000);
    
    tft_command(0x29); // DISPON
    usleep(100000);
}

void tft_shutdown(void) {
    if (spi_fd >= 0) close(spi_fd);
    if (tft_gpio_map != MAP_FAILED) munmap((void*)tft_gpio_map, BLOCK_SIZE);
}

void tft_sleep(int sleep_mode) {
    if (sleep_mode) {
        if (tft_gpio_map != MAP_FAILED) GPIO_CLR = 1 << LED_PIN;
        tft_command(0x28); // DISPOFF
        tft_command(0x10); // SLPIN
    } else {
        if (tft_gpio_map != MAP_FAILED) GPIO_SET = 1 << LED_PIN;
        tft_command(0x11); // SLPOUT
        usleep(120000);    // Wait 120ms
        tft_command(0x29); // DISPON
    }
}

int get_button_state(void) {
    if (tft_gpio_map == MAP_FAILED) return 1; // Default HIGH (unpressed)
    // GPLEV0 is at word offset 13 (0x34 bytes)
    return (*(tft_gpio_map + 13) & (1 << BUTTON_PIN)) ? 1 : 0;
}

void tft_fill(uint16_t color) {
    for (int i = 0; i < TFT_BUFFER_SIZE; i++) {
        tft_buffer[i] = color;
    }
}

void tft_update(void) {
    tft_command(0x2A); // CASET
    tft_data(0x00); tft_data(0x00);
    tft_data(0x00); tft_data(0x9F); // 159
    
    tft_command(0x2B); // RASET
    tft_data(0x00); tft_data(0x00);
    tft_data(0x00); tft_data(0x7F); // 127
    
    tft_command(0x2C); // RAMWR
    
    // Swap bytes for ST7735 (big endian)
    uint8_t buf[TFT_BUFFER_SIZE * 2];
    for (int i = 0; i < TFT_BUFFER_SIZE; i++) {
        buf[i*2] = tft_buffer[i] >> 8;
        buf[i*2 + 1] = tft_buffer[i] & 0xFF;
    }
    
    dc_high();
    
    // SPI transfers usually have a limit (4096 bytes is typical)
    int chunk_size = 4096;
    for (int i = 0; i < sizeof(buf); i += chunk_size) {
        int len = sizeof(buf) - i;
        if (len > chunk_size) len = chunk_size;
        spi_write(&buf[i], len);
    }
}

void tft_draw_pixel(int x, int y, uint16_t color) {
    if (x >= 0 && x < TFT_WIDTH && y >= 0 && y < TFT_HEIGHT) {
        tft_buffer[y * TFT_WIDTH + x] = color;
    }
}

static int get_char_index_5x7(char c) {
    for (int i = 0; i < NUM_CHARS_5X7; i++) {
        if (CHARS_5X7[i] == c) return i;
    }
    return -1;
}

static int get_char_index_32x48(char c) {
    for (int i = 0; i < NUM_CHARS_32X48; i++) {
        if (CHARS_32X48[i] == c) return i;
    }
    return -1;
}

void tft_draw_text(int x, int y, const char* str, uint16_t color, int scale) {
    int cx = x;
    while (*str) {
        char c = *str++;
        int idx = get_char_index_5x7(c);
        if (idx >= 0) {
            for (int r = 0; r < 7; r++) {
                uint8_t row = FONT_5X7[idx][r];
                for (int col = 0; col < 5; col++) {
                    if ((row >> (4 - col)) & 1) {
                        if (scale == 1) {
                            tft_draw_pixel(cx + col, y + r, color);
                        } else {
                            for (int dy = 0; dy < scale; dy++) {
                                for (int dx = 0; dx < scale; dx++) {
                                    tft_draw_pixel(cx + col * scale + dx, y + r * scale + dy, color);
                                }
                            }
                        }
                    }
                }
            }
        }
        cx += (5 + 1) * scale;
    }
}

// Forward declare interpolate_color
static uint16_t interpolate_color(uint16_t c1, uint16_t c2, float t);

void tft_draw_text_gradient(int x, int y, const char* str, uint16_t top_color, uint16_t bot_color) {
    int cx = x;
    while (*str) {
        char c = *str++;
        int idx = (int)c - 32;
        if (idx >= 0 && idx < 96) {
            for (int r = 0; r < 12; r++) {
                uint8_t row = FONT_8X12[idx][r];
                float t = (float)r / 11.0f;
                uint16_t color = interpolate_color(top_color, bot_color, t);
                for (int col = 0; col < 8; col++) {
                    if ((row >> (7 - col)) & 1) {
                        tft_draw_pixel(cx + col, y + r, color);
                    }
                }
            }
        }
        cx += 8;
    }
}

static uint16_t interpolate_color(uint16_t c1, uint16_t c2, float t) {
    int r1 = (c1 >> 11) & 0x1F;
    int g1 = (c1 >> 5) & 0x3F;
    int b1 = c1 & 0x1F;
    
    int r2 = (c2 >> 11) & 0x1F;
    int g2 = (c2 >> 5) & 0x3F;
    int b2 = c2 & 0x1F;
    
    int r = r1 + (r2 - r1) * t;
    int g = g1 + (g2 - g1) * t;
    int b = b1 + (b2 - b1) * t;
    
    return (r << 11) | (g << 5) | b;
}

void tft_draw_highres_text(int x, int y, const char* text, uint16_t top_color, uint16_t bot_color, uint16_t outline_color) {
    int cx = x;
    int height = 48;
    int width = 32;
    
    while (*text) {
        char c = *text++;
        int idx = get_char_index_32x48(c);
        if (idx >= 0) {
            // Draw outline
            for (int r = 0; r < height; r++) {
                uint32_t row = FONT_32X48[idx][r];
                for (int col = 0; col < width; col++) {
                    if ((row >> (31 - col)) & 1) {
                        // 1px border
                        tft_draw_pixel(cx + col - 1, y + r, outline_color);
                        tft_draw_pixel(cx + col + 1, y + r, outline_color);
                        tft_draw_pixel(cx + col, y + r - 1, outline_color);
                        tft_draw_pixel(cx + col, y + r + 1, outline_color);
                        
                        // Diagonal borders
                        tft_draw_pixel(cx + col - 1, y + r - 1, outline_color);
                        tft_draw_pixel(cx + col + 1, y + r - 1, outline_color);
                        tft_draw_pixel(cx + col - 1, y + r + 1, outline_color);
                        tft_draw_pixel(cx + col + 1, y + r + 1, outline_color);
                    }
                }
            }
            
            // Draw inner text
            for (int r = 0; r < height; r++) {
                uint32_t row = FONT_32X48[idx][r];
                float t = (float)r / height;
                uint16_t color = interpolate_color(top_color, bot_color, t);
                
                for (int col = 0; col < width; col++) {
                    if ((row >> (31 - col)) & 1) {
                        tft_draw_pixel(cx + col, y + r, color);
                    }
                }
            }
        }
        cx += width + 2; // slight spacing
    }
}
