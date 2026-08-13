#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include "tft.h"
#include "max7219.h"
#include "weather.h"
#include "anim.h"

static bool running = true;

void handle_sigint(int sig) {
    running = false;
}

static const uint8_t SEGMENT_MAP[128] = {
    [' '] = 0x00, ['-'] = 0x01,
    ['0'] = 0x7E, ['1'] = 0x30, ['2'] = 0x6D, ['3'] = 0x79,
    ['4'] = 0x33, ['5'] = 0x5B, ['6'] = 0x5F, ['7'] = 0x70,
    ['8'] = 0x7F, ['9'] = 0x7B,
    ['A'] = 0x77, ['P'] = 0x67,
    ['C'] = 0x4E, ['F'] = 0x47,
};

int main(void) {
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    printf("Starting C DeskClock...\n");
    
    tft_init();
    max7219_init();
    anim_init();
    weather_init();
    
    int last_sec = -1;
    
    while (running) {
        time_t rawtime;
        time(&rawtime);
        struct tm timeinfo_buf;
        struct tm * timeinfo = localtime_r(&rawtime, &timeinfo_buf);
        
        if (last_sec != timeinfo->tm_sec) {
            last_sec = timeinfo->tm_sec;
            
            int hour = timeinfo->tm_hour;
            int min = timeinfo->tm_min;
            int is_pm = (hour >= 12);
            if (hour == 0) hour = 12;
            if (hour > 12) hour -= 12;
            
            char time_str[32];
            snprintf(time_str, sizeof(time_str), "%2d %02d  %c", hour, min, is_pm ? 'P' : 'A');
            
            time_t manila_raw = rawtime + (8 * 3600); // UTC+8
            struct tm manila_info_buf;
            struct tm * manila_info = gmtime_r(&manila_raw, &manila_info_buf);
            
            int m_hour = manila_info->tm_hour;
            int m_min = manila_info->tm_min;
            int m_is_pm = (m_hour >= 12);
            if (m_hour == 0) m_hour = 12;
            if (m_hour > 12) m_hour -= 12;
            
            char time_str_bot[32];
            snprintf(time_str_bot, sizeof(time_str_bot), "%2d %02d  %c", m_hour, m_min, m_is_pm ? 'P' : 'A');
            
            uint8_t digits_top[8] = {0};
            uint8_t digits_bot[8] = {0};
            
            for (int i = 0; i < 8; i++) {
                digits_top[i] = SEGMENT_MAP[(int)time_str[i]];
                digits_bot[i] = SEGMENT_MAP[(int)time_str_bot[i]];
            }
            
            for (int i = 0; i < 8; i++) {
                uint8_t data[2] = {digits_top[i], digits_bot[i]};
                max7219_write_cmd_chain(8 - i, data, 2);
            }
        }
        
        bool is_night = (timeinfo->tm_hour < 6 || timeinfo->tm_hour >= 18);
        
        pthread_mutex_lock(&g_weather_data.mutex);
        char temp[16], desc[64], hum[16];
        strcpy(temp, g_weather_data.temp);
        strcpy(desc, g_weather_data.desc);
        strcpy(hum, g_weather_data.hum);
        pthread_mutex_unlock(&g_weather_data.mutex);
        
        for (int i = 0; desc[i]; i++) {
            desc[i] = toupper((unsigned char)desc[i]);
        }
        
        bool DEMO_MODE = true;
        if (DEMO_MODE) {
            const char* test_modes[] = {"CLEAR", "RAIN", "SNOW", "CLOUD"};
            strcpy(desc, test_modes[(rawtime / 5) % 4]);
        }
        
        anim_draw(desc, is_night);
        
        uint16_t temp_color_top, temp_color_bot, outline_color, unit_color;
        if (strstr(desc, "RAIN") || strstr(desc, "DRIZZLE") || strstr(desc, "THUNDER")) {
            temp_color_top = 0x07FF; temp_color_bot = 0x001F;
            unit_color = 0x07FF; outline_color = 0xFFFF;
        } else if (strstr(desc, "SNOW")) {
            temp_color_top = 0xFFFF; temp_color_bot = 0x841F;
            unit_color = 0xFFFF; outline_color = 0xFFFF;
        } else if (strstr(desc, "CLOUD")) {
            temp_color_top = 0xCE79; temp_color_bot = 0xFFFF;
            unit_color = 0xCE79; outline_color = 0x0000;
        } else {
            if (is_night) {
                temp_color_top = 0x07FF; temp_color_bot = 0xF81F;
                unit_color = 0x07FF; outline_color = 0x0000;
            } else {
                temp_color_top = 0xFFE0; temp_color_bot = 0xFC00;
                unit_color = 0xFFE0; outline_color = 0x0000;
            }
        }
        
        int num_start_x = 40;
        if (strlen(temp) == 4) num_start_x = 30;
        else if (strlen(temp) == 3) num_start_x = 50;
        
        char temp_num[16];
        strcpy(temp_num, temp);
        if (strlen(temp_num) > 0 && temp_num[strlen(temp_num)-1] == 'C') {
            temp_num[strlen(temp_num)-1] = '\0';
        }
        
        tft_draw_highres_text(num_start_x, 40, temp_num, temp_color_top, temp_color_bot, outline_color);
        int unit_x = num_start_x + (strlen(temp_num) * 34) + 2;
        tft_draw_text(unit_x, 40 + (48 - 14), " C", unit_color, 2);
        
        // Degree symbol (hollow ring)
        int ring_y = 40 + (48 - 14) + 2;
        tft_draw_pixel(unit_x+2, ring_y, unit_color);
        tft_draw_pixel(unit_x+3, ring_y, unit_color);
        tft_draw_pixel(unit_x+1, ring_y+1, unit_color);
        tft_draw_pixel(unit_x+4, ring_y+1, unit_color);
        tft_draw_pixel(unit_x+1, ring_y+2, unit_color);
        tft_draw_pixel(unit_x+4, ring_y+2, unit_color);
        tft_draw_pixel(unit_x+2, ring_y+3, unit_color);
        tft_draw_pixel(unit_x+3, ring_y+3, unit_color);
        
        char hum_text[32];
        snprintf(hum_text, sizeof(hum_text), "HUM %s", hum);
        int hum_w = strlen(hum_text) * 6;
        int hum_x = (160 - hum_w) / 2;
        if (hum_x < 0) hum_x = 0;
        
        tft_draw_text_gradient(hum_x, 115, hum_text, 0xFFE0, 0x07E0, 1); // Yellow to Green
        
        tft_update();
        
        usleep(100000); // 10 FPS
    }
    
    printf("Shutting down...\n");
    weather_shutdown();
    tft_shutdown();
    max7219_shutdown();
    
    return 0;
}
