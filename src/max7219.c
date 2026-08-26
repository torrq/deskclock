#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "max7219.h"
#include "config.h"

#define BCM2708_PERI_BASE 0x3F000000 
#define GPIO_BASE 0x200000 
#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

static volatile uint32_t *gpio_map = MAP_FAILED;

#define INP_GPIO(g) *(gpio_map+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio_map+((g)/10)) |=  (1<<(((g)%10)*3))

#define GPIO_SET *(gpio_map+7)
#define GPIO_CLR *(gpio_map+10)

#define DIN_PIN 17
#define CLK_PIN 18
#define CS_PIN 23

static void setup_io() {
    int mem_fd;
    if ((mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
        perror("can't open /dev/gpiomem");
        exit(-1);
    }
    gpio_map = (uint32_t *)mmap(
        NULL,
        BLOCK_SIZE,
        PROT_READ|PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        0
    );
    close(mem_fd);

    if (gpio_map == MAP_FAILED) {
        perror("mmap error");
        exit(-1);
    }
}

static inline void set_pin(int pin, int val) {
    if (val) {
        GPIO_SET = 1 << pin;
    } else {
        GPIO_CLR = 1 << pin;
    }
}

static inline void delay_ns(void) {
    // 50ns minimum for MAX7219, a small volatile loop is sufficient
    for(volatile int k = 0; k < 5; k++);
}

static inline void shift_out(uint8_t val) {
    for (int i = 0; i < 8; i++) {
        // MSB first
        if (val & (1 << (7 - i))) {
            GPIO_SET = 1 << DIN_PIN;
        } else {
            GPIO_CLR = 1 << DIN_PIN;
        }
        delay_ns();
        
        // Toggle CLK
        GPIO_SET = 1 << CLK_PIN;
        delay_ns();
        GPIO_CLR = 1 << CLK_PIN;
        delay_ns();
    }
}

void max7219_init(void) {
    setup_io();
    
    INP_GPIO(DIN_PIN); OUT_GPIO(DIN_PIN);
    INP_GPIO(CLK_PIN); OUT_GPIO(CLK_PIN);
    INP_GPIO(CS_PIN); OUT_GPIO(CS_PIN);
    
    set_pin(CS_PIN, 1);
    set_pin(CLK_PIN, 0);
    set_pin(DIN_PIN, 0);
    
    // Initialize matrices
    max7219_send_command_all(0x09, 0x00, g_max7219_displays); // No decode
    max7219_send_command_all(0x0A, 0x01, g_max7219_displays); // Low intensity
    max7219_send_command_all(0x0B, 0x07, g_max7219_displays); // Scan limit 8
    max7219_send_command_all(0x0C, 0x01, g_max7219_displays); // Normal operation
    max7219_send_command_all(0x0F, 0x00, g_max7219_displays); // Disable display test
    max7219_clear(g_max7219_displays);
}

void max7219_write_cmd_chain(uint8_t reg, uint8_t data_list[], int num_displays) {
    GPIO_CLR = 1 << CS_PIN;
    for (int i = num_displays - 1; i >= 0; i--) {
        shift_out(reg);
        shift_out(data_list[i]);
    }
    GPIO_SET = 1 << CS_PIN;
}

void max7219_send_command_all(uint8_t reg, uint8_t value, int num_displays) {
    GPIO_CLR = 1 << CS_PIN;
    for (int i = 0; i < num_displays; i++) {
        shift_out(reg);
        shift_out(value);
    }
    GPIO_SET = 1 << CS_PIN;
}

void max7219_clear(int num_displays) {
    for (int i = 1; i <= 8; i++) {
        max7219_send_command_all(i, 0x00, num_displays);
    }
}

void max7219_shutdown(void) {
    max7219_clear(g_max7219_displays);
    if (gpio_map != MAP_FAILED) {
        munmap((void*)gpio_map, BLOCK_SIZE);
    }
}

void max7219_sleep(int sleep_mode, int num_displays) {
    max7219_send_command_all(0x0C, sleep_mode ? 0x00 : 0x01, num_displays);
}
