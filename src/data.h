#ifndef DATA_H
#define DATA_H

#include <stdint.h>

extern const uint8_t SEGMENT_MAP[128];

extern const int NUM_CHARS_5X7;
extern const char CHARS_5X7[];
extern const uint8_t FONT_5X7[40][7];

extern const int NUM_CHARS_32X48;
extern const char CHARS_32X48[];
extern const uint32_t FONT_32X48[11][48];

extern const uint8_t FONT_8X12[96][12];

#endif
