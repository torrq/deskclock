#ifndef WEATHER_H
#define WEATHER_H

#include <stdbool.h>
#include <pthread.h>

typedef struct {
    char temp[16];
    char desc[64];
    char hum[16];
    bool updated;
    pthread_mutex_t mutex;
} WeatherData;

extern WeatherData g_weather_data;

void weather_init(void);
void weather_shutdown(void);

#endif
