#ifndef CONFIG_H
#define CONFIG_H

extern char g_weather_url[256];
extern char g_camera_url[256];
extern int g_bottom_clock_offset;

void config_load(const char* filepath);

#endif // CONFIG_H
