#ifndef MAX7219_H
#define MAX7219_H

#include <stdint.h>

void max7219_init(void);
void max7219_shutdown(void);
void max7219_write_cmd_chain(uint8_t reg, uint8_t data_list[], int num_displays);
void max7219_send_command_all(uint8_t reg, uint8_t value, int num_displays);
void max7219_clear(int num_displays);
void max7219_sleep(int sleep_mode, int num_displays);

#endif
