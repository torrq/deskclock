#ifndef CONFIG_H
#define CONFIG_H

extern int g_start_screen; // 0=animations, 1=camera
extern char g_weather_url[256];

extern char g_camera_url[256];
extern int g_camera_refresh_interval;

extern int g_max7219_displays;
extern int g_bottom_clock_offset;
extern float g_camera_gamma;
extern int g_camera_scale_mode; // 0=stretch, 1=crop
extern int g_camera_crop_top;
extern int g_camera_crop_bottom;
extern int g_camera_crop_left;
extern int g_camera_crop_right;

void config_load(const char* filepath);

#endif // CONFIG_H
