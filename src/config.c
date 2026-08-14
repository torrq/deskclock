#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char g_weather_url[256] = "http://wttr.in/Vancouver?format=j1";
char g_camera_url[256] = "https://trafficcams.vancouver.ca/cameraimages/BurrardCanadaWest.jpg";
int g_bottom_clock_offset = 8;

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
            }
        }
    }
    fclose(fp);
    printf("Loaded configuration from %s\n", filepath);
}
