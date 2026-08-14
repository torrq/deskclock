#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char g_weather_url[256] = "http://wttr.in/Vancouver?format=j1";
char g_camera_url[256] = "https://trafficcams.vancouver.ca/cameraimages/BurrardCanadaWest.jpg";
int g_bottom_clock_offset = 8;
float g_camera_gamma = 2.2f;
int g_camera_scale_mode = 0; // 0=stretch, 1=crop
int g_camera_crop_top = 0;
int g_camera_crop_bottom = 0;
int g_camera_crop_left = 0;
int g_camera_crop_right = 0;

static void trim_whitespace(char* str) {
    char* end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

void config_load(const char* filepath) {
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        printf("Failed to open config file: %s (Using defaults)\n", filepath);
        return;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        char* delim = strchr(line, '=');
        if (delim) {
            *delim = '\0';
            char* key = line;
            char* value = delim + 1;
            
            trim_whitespace(key);
            trim_whitespace(value);
            
            if (strcmp(key, "WEATHER_URL") == 0) {
                strncpy(g_weather_url, value, sizeof(g_weather_url) - 1);
                g_weather_url[sizeof(g_weather_url) - 1] = '\0';
            } else if (strcmp(key, "CAMERA_URL") == 0) {
                strncpy(g_camera_url, value, sizeof(g_camera_url) - 1);
                g_camera_url[sizeof(g_camera_url) - 1] = '\0';
            } else if (strcmp(key, "BOTTOM_CLOCK_UTC_OFFSET") == 0) {
                g_bottom_clock_offset = atoi(value);
            } else if (strcmp(key, "CAMERA_GAMMA") == 0) {
                g_camera_gamma = atof(value);
            } else if (strcmp(key, "CAMERA_SCALE_MODE") == 0) {
                if (strcmp(value, "crop") == 0) g_camera_scale_mode = 1;
                else g_camera_scale_mode = 0;
            } else if (strcmp(key, "CAMERA_CROP_TOP") == 0) {
                g_camera_crop_top = atoi(value);
            } else if (strcmp(key, "CAMERA_CROP_BOTTOM") == 0) {
                g_camera_crop_bottom = atoi(value);
            } else if (strcmp(key, "CAMERA_CROP_LEFT") == 0) {
                g_camera_crop_left = atoi(value);
            } else if (strcmp(key, "CAMERA_CROP_RIGHT") == 0) {
                g_camera_crop_right = atoi(value);
            }
        }
    }
    fclose(fp);
    printf("Loaded configuration from %s\n", filepath);
}
