#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

extern uint16_t camera_frame[160 * 128];
extern pthread_mutex_t camera_mutex;
extern bool camera_updated;

void camera_init(void);
void camera_shutdown(void);

#endif
