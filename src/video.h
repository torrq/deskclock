#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define VIDEO_WIDTH 160
#define VIDEO_HEIGHT 128
#define VIDEO_FRAME_SIZE (VIDEO_WIDTH * VIDEO_HEIGHT)

extern uint16_t video_frame[VIDEO_FRAME_SIZE];
extern pthread_mutex_t video_mutex;
extern bool video_updated;

void video_init(void);
void video_shutdown(void);

#endif
