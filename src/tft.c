#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "tft.h"
#include "fonts.h"

// We use GPIO 24 for Data/Command control.
// This requires it to be exported via sysfs before running, or we export it here.
#define DC_PIN 24
#define RST_PIN 25

static int spi_fd = -1;
static int dc_fd = -1;
static int rst_fd = -1;

uint16_t tft_buffer[TFT_BUFFER_SIZE];

static int gpio_export(int pin) {
    char path[64];
    int fd;
    
    // Export pin
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        perror("Failed to open /sys/class/gpio/export (are you running with sudo?)");
        return -1;
    }
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", pin);
    write(fd, buf, strlen(buf));
    close(fd);
    
    // Set direction to out
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    usleep(100000); // wait for udev
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        printf("Failed to open %s\n", path);
        return -1;
    }
    write(fd, "out", 3);
    close(fd);
    
    // Open value file
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        printf("Failed to open %s\n", path);
    }
    return fd;
}

static void dc_high(void) {
    if (dc_fd >= 0) write(dc_fd, "1", 1);
}

static void dc_low(void) {
    if (dc_fd >= 0) write(dc_fd, "0", 1);
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
    dc_fd = gpio_export(DC_PIN);
    rst_fd = gpio_export(RST_PIN);
    
    // Hardware reset
    if (rst_fd >= 0) write(rst_fd, "1", 1);
    usleep(50000);
    if (rst_fd >= 0) write(rst_fd, "0", 1);
    usleep(50000);
    if (rst_fd >= 0) write(rst_fd, "1", 1);
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

    // Init sequence for ST7735
    tft_command(0x01); // SWRESET
    usleep(150000);
    
    tft_command(0x11); // SLPOUT
    usleep(500000);
    
    tft_command(0x3A); // COLMOD
    tft_data(0x05);    // 16-bit color
    
    tft_command(0x36); // MADCTL
    tft_data(0x60);    // Landscape
    
    tft_command(0x29); // DISPON
    usleep(100000);
}

void tft_shutdown(void) {
    if (spi_fd >= 0) close(spi_fd);
    if (dc_fd >= 0) close(dc_fd);
    if (rst_fd >= 0) close(rst_fd);
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

void tft_draw_text(int x, int y, const char* text, uint16_t color, int scale) {
    int cx = x;
    while (*text) {
        char c = *text++;
        int idx = get_char_index_5x7(c);
        if (idx >= 0) {
            for (int r = 0; r < 7; r++) {
                uint8_t row = FONT_5X7[idx][r];
                for (int col = 0; col < 5; col++) {
                    if ((row >> (4 - col)) & 1) {
                        for (int dy = 0; dy < scale; dy++) {
                            for (int dx = 0; dx < scale; dx++) {
                                tft_draw_pixel(cx + col * scale + dx, y + r * scale + dy, color);
                            }
                        }
                    }
                }
            }
        }
        cx += 6 * scale;
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
