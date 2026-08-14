#ifndef CONSTANTS_H
#define CONSTANTS_H

#define DEFAULT_START_SCREEN 0 // 0=animations, 1=camera
#define DEFAULT_WEATHER_URL "http://wttr.in/Vancouver?format=j1"

#define DEFAULT_CAMERA_ENGINE "url" // "url" or "command"
#define DEFAULT_CAMERA_URL "https://imgproxy.windy.com/_/full/plain/current/1575920021/original.jpg"
#define DEFAULT_CAMERA_COMMAND "/home/dietpi/get_camera.sh"
#define DEFAULT_CAMERA_COMMAND_OUTPUT "/tmp/deskclock_camera.jpg"
#define DEFAULT_CAMERA_REFRESH_INTERVAL 300

#define DEFAULT_BOTTOM_CLOCK_OFFSET 8
#define DEFAULT_CAMERA_GAMMA 2.2f
#define DEFAULT_CAMERA_SCALE_MODE 0 // 0=stretch, 1=crop
#define DEFAULT_CAMERA_CROP_TOP 0
#define DEFAULT_CAMERA_CROP_BOTTOM 0
#define DEFAULT_CAMERA_CROP_LEFT 0
#define DEFAULT_CAMERA_CROP_RIGHT 0

#endif
