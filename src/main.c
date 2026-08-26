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
#include "camera.h"
#include "video.h"
#include "config.h"
#include "data.h"

static bool running = true;

enum TFTMode {
    MODE_ANIM = 0,
    MODE_CAM_TEXT = 1,
    MODE_CAM_CLEAN = 2,
    MODE_LIVE_VIDEO = 3,
    MODE_OFF = 4
};

#define NUM_MODES 5
static int mode_stack[NUM_MODES] = {MODE_ANIM, MODE_CAM_TEXT, MODE_CAM_CLEAN, MODE_LIVE_VIDEO, MODE_OFF};
static int display_mode_idx = 0;

int get_current_mode() {
    return mode_stack[display_mode_idx];
}

void init_mode_stack() {
    if (g_start_screen == 2) { // stream / live video
        mode_stack[0] = MODE_LIVE_VIDEO;
        mode_stack[1] = MODE_OFF;
        mode_stack[2] = MODE_ANIM;
        mode_stack[3] = MODE_CAM_TEXT;
        mode_stack[4] = MODE_CAM_CLEAN;
    } else if (g_start_screen == 1) { // camera snapshot
        mode_stack[0] = MODE_CAM_TEXT;
        mode_stack[1] = MODE_CAM_CLEAN;
        mode_stack[2] = MODE_LIVE_VIDEO;
        mode_stack[3] = MODE_OFF;
        mode_stack[4] = MODE_ANIM;
    } else { // animations
        mode_stack[0] = MODE_ANIM;
        mode_stack[1] = MODE_CAM_TEXT;
        mode_stack[2] = MODE_CAM_CLEAN;
        mode_stack[3] = MODE_LIVE_VIDEO;
        mode_stack[4] = MODE_OFF;
    }
}

void handle_sigint(int sig) {
    running = false;
}

void apply_display_mode() {
    switch(get_current_mode()) {
        case MODE_ANIM:
        case MODE_CAM_TEXT:
        case MODE_CAM_CLEAN:
        case MODE_LIVE_VIDEO:
            tft_sleep(0);
            break;
        case MODE_OFF:
            tft_sleep(1);
            break;
    }
}

void handle_sigusr1(int sig) {
    display_mode_idx = (display_mode_idx + 1) % NUM_MODES;
    apply_display_mode();
}

int main(void) {
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
    signal(SIGUSR1, handle_sigusr1);

    // Load config before initializing subsystems
    config_load("config.cfg");

    printf("Starting C DeskClock...\n");
    
    tft_init();
    max7219_init();
    
    init_mode_stack();
    apply_display_mode();
    
    anim_init();
    weather_init();
    camera_init();
    video_init();
    
    int last_sec = -1;
    int last_btn_state = 1;
    
    while (running) {
        int btn = get_button_state();
        if (btn == 0 && last_btn_state == 1) { // Falling edge (pressed)
            handle_sigusr1(SIGUSR1);
        }
        last_btn_state = btn;
        
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
            snprintf(time_str, sizeof(time_str), "%2d-%02d  %c", hour, min, is_pm ? 'P' : 'A');
            
            time_t bot_raw = rawtime + (g_bottom_clock_offset * 3600);
            struct tm bot_info_buf;
            struct tm * bot_info = gmtime_r(&bot_raw, &bot_info_buf);
            
            int m_hour = bot_info->tm_hour;
            int m_min = bot_info->tm_min;
            int m_is_pm = (m_hour >= 12);
            if (m_hour == 0) m_hour = 12;
            if (m_hour > 12) m_hour -= 12;
            
            char time_str_bot[32];
            snprintf(time_str_bot, sizeof(time_str_bot), "%2d-%02d  %c", m_hour, m_min, m_is_pm ? 'P' : 'A');
            
            // Panel 2 (3rd Display): Date (YY-MM-DD, e.g. "26-08-26")
            char date_str[32];
            snprintf(date_str, sizeof(date_str), "%02d-%02d-%02d",
                     (timeinfo->tm_year + 1900) % 100,
                     timeinfo->tm_mon + 1,
                     timeinfo->tm_mday);
            
            // Panel 3 (4th Display): Weather Temp & Humidity (e.g. " 22C 65H")
            char weather_str[32] = "        ";
            pthread_mutex_lock(&g_weather_data.mutex);
            if (g_weather_data.updated) {
                char temp_buf[16] = {0};
                char hum_buf[16] = {0};
                strncpy(temp_buf, g_weather_data.temp, sizeof(temp_buf) - 1);
                strncpy(hum_buf, g_weather_data.hum, sizeof(hum_buf) - 1);
                int hum_val = atoi(hum_buf);
                snprintf(weather_str, sizeof(weather_str), "%4s %2dH", temp_buf, hum_val);
            } else {
                snprintf(weather_str, sizeof(weather_str), "---  --H");
            }
            pthread_mutex_unlock(&g_weather_data.mutex);
            
            uint8_t digits[4][8] = {{0}};
            for (int i = 0; i < 8; i++) {
                digits[0][i] = SEGMENT_MAP[(uint8_t)time_str[i]];
                digits[1][i] = SEGMENT_MAP[(uint8_t)time_str_bot[i]];
                digits[2][i] = SEGMENT_MAP[(uint8_t)date_str[i]];
                digits[3][i] = SEGMENT_MAP[(uint8_t)weather_str[i]];
            }
            
            for (int i = 0; i < 8; i++) {
                uint8_t data[4] = {0};
                for (int d = 0; d < g_max7219_displays && d < 4; d++) {
                    data[d] = digits[d][i];
                }
                max7219_write_cmd_chain(8 - i, data, g_max7219_displays);
            }
        }
        
        if (get_current_mode() == MODE_CAM_TEXT || get_current_mode() == MODE_CAM_CLEAN) {
            pthread_mutex_lock(&camera_mutex);
            memcpy(tft_buffer, camera_frame, sizeof(camera_frame));
            if (!camera_updated) {
                tft_draw_text(40, 60, "FETCHING CAM", 0xFFFF, 1);
            }
            pthread_mutex_unlock(&camera_mutex);
            
            if (get_current_mode() == MODE_CAM_TEXT) {
                // Draw weather overlay
                pthread_mutex_lock(&g_weather_data.mutex);
                char temp[16], hum[16];
                strcpy(temp, g_weather_data.temp);
                strcpy(hum, g_weather_data.hum);
                pthread_mutex_unlock(&g_weather_data.mutex);
                
                char temp_num[16];
                strcpy(temp_num, temp);
                if (strlen(temp_num) > 0 && temp_num[strlen(temp_num)-1] == 'C') {
                    temp_num[strlen(temp_num)-1] = '\0';
                }
                
                int num_start_x = 40;
                if (strlen(temp) == 4) num_start_x = 30;
                else if (strlen(temp) == 3) num_start_x = 50;
                
                // White text with black outline for visibility
                tft_draw_highres_text(num_start_x, 40, temp_num, 0xFFFF, 0xFFFF, 0x0000);
                
                int unit_x = num_start_x + (strlen(temp_num) * 34) + 2;
                int ring_y = 40 + (48 - 14) + 2;
                
                // Unit 'C' with black drop shadow
                tft_draw_text(unit_x + 1, 40 + (48 - 14) + 1, " C", 0x0000, 2);
                tft_draw_text(unit_x, 40 + (48 - 14), " C", 0xFFFF, 2);
                
                // Degree symbol shadow
                tft_draw_pixel(unit_x+2+1, ring_y+1, 0x0000);
                tft_draw_pixel(unit_x+3+1, ring_y+1, 0x0000);
                tft_draw_pixel(unit_x+1+1, ring_y+1+1, 0x0000);
                tft_draw_pixel(unit_x+4+1, ring_y+1+1, 0x0000);
                tft_draw_pixel(unit_x+1+1, ring_y+2+1, 0x0000);
                tft_draw_pixel(unit_x+4+1, ring_y+2+1, 0x0000);
                tft_draw_pixel(unit_x+2+1, ring_y+3+1, 0x0000);
                tft_draw_pixel(unit_x+3+1, ring_y+3+1, 0x0000);
                
                // Degree symbol
                tft_draw_pixel(unit_x+2, ring_y, 0xFFFF);
                tft_draw_pixel(unit_x+3, ring_y, 0xFFFF);
                tft_draw_pixel(unit_x+1, ring_y+1, 0xFFFF);
                tft_draw_pixel(unit_x+4, ring_y+1, 0xFFFF);
                tft_draw_pixel(unit_x+1, ring_y+2, 0xFFFF);
                tft_draw_pixel(unit_x+4, ring_y+2, 0xFFFF);
                tft_draw_pixel(unit_x+2, ring_y+3, 0xFFFF);
                tft_draw_pixel(unit_x+3, ring_y+3, 0xFFFF);
                
                // Humidity with black drop shadow
                char hum_text[32];
                snprintf(hum_text, sizeof(hum_text), "HUM %s", hum);
                int hum_w = strlen(hum_text) * 12; // 2x scaled 5x7 font
                int hum_x = (160 - hum_w) / 2;
                if (hum_x < 0) hum_x = 0;
                
                tft_draw_text(hum_x + 1, 114 + 1, hum_text, 0x0000, 2);
                tft_draw_text(hum_x, 114, hum_text, 0xFFFF, 2);
            }
            
            tft_update();
        } else if (get_current_mode() == MODE_ANIM) {
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
            
            bool DEMO_MODE = false;
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
            int hum_w = strlen(hum_text) * 12; // 2x scaled 5x7 font (6px width * 2 = 12)
            int hum_x = (160 - hum_w) / 2;
            if (hum_x < 0) hum_x = 0;
            
            tft_draw_text(hum_x, 114, hum_text, 0x07FF, 2); // Cyan, 2x scale
            
            tft_update();
        } else if (get_current_mode() == MODE_LIVE_VIDEO) {
            pthread_mutex_lock(&video_mutex);
            memcpy(tft_buffer, video_frame, sizeof(video_frame));
            if (!video_updated) {
                tft_draw_text(30, 60, "STARTING STREAM", 0xFFFF, 1);
            }
            pthread_mutex_unlock(&video_mutex);
            tft_update();
        }
        
        usleep(100000); // 10 FPS
    }
    
    printf("Shutting down...\n");
    video_shutdown();
    camera_shutdown();
    weather_shutdown();
    tft_shutdown();
    max7219_shutdown();
    
    return 0;
}
