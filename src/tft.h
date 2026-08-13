#ifndef TFT_H
#define TFT_H

#include <stdint.h>
#include <stdbool.h>

#define TFT_WIDTH 160
#define TFT_HEIGHT 128
#define TFT_BUFFER_SIZE (TFT_WIDTH * TFT_HEIGHT)

extern uint16_t tft_buffer[TFT_BUFFER_SIZE];

void tft_init(void);
void tft_shutdown(void);
void tft_update(void);
void tft_fill(uint16_t color);
void tft_draw_pixel(int x, int y, uint16_t color);
void tft_draw_text(int x, int y, const char* text, uint16_t color, int scale);
void tft_draw_text_gradient(int x, int y, const char* str, uint16_t top_color, uint16_t bot_color, int scale);
void tft_draw_highres_text(int x, int y, const char* text, uint16_t top_color, uint16_t bot_color, uint16_t outline_color);

#endif
