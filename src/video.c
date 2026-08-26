#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "video.h"
#include "config.h"

uint16_t video_frame[VIDEO_FRAME_SIZE];
pthread_mutex_t video_mutex = PTHREAD_MUTEX_INITIALIZER;
bool video_updated = false;

static pthread_t video_thread;
static bool running = false;

extern int get_current_mode(void);

static void* video_loop(void* arg) {
    uint16_t frame_buf[VIDEO_FRAME_SIZE];
    
    while (running) {
        // Only run video decoding pipeline when in MODE_LIVE_VIDEO (3)
        if (get_current_mode() != 3) {
            usleep(100000); // 100ms
            continue;
        }

        printf("[Video Engine] Starting live stream pipeline at %d FPS...\n", g_video_fps);
        
        char pipeline_cmd[4096];
        if (strstr(g_camera_url, "youtube.com") || strstr(g_camera_url, "youtu.be")) {
            snprintf(pipeline_cmd, sizeof(pipeline_cmd),
                "yt-dlp --extractor-args \"youtube:player_client=android,web\" -f worst --no-warnings -o - \"%s\" 2>/dev/null | "
                "ffmpeg -nostdin -nostats -loglevel error -i pipe:0 "
                "-vf \"fps=%d,scale=160:128\" "
                "-f rawvideo -pix_fmt rgb565le -",
                g_camera_url, g_video_fps);
        } else {
            snprintf(pipeline_cmd, sizeof(pipeline_cmd),
                "ffmpeg -nostdin -nostats -loglevel error -i \"%s\" "
                "-vf \"fps=%d,scale=160:128\" "
                "-f rawvideo -pix_fmt rgb565le -",
                g_camera_url, g_video_fps);
        }
        
        FILE* pipe = popen(pipeline_cmd, "r");
        if (!pipe) {
            printf("[Video Engine] Failed to start pipeline.\n");
            for (int i = 0; i < 5 && running && get_current_mode() == 3; i++) sleep(1);
            continue;
        }
        
        // Read raw video frames continuously while in video mode
        const size_t target_bytes = VIDEO_FRAME_SIZE * sizeof(uint16_t);
        while (running && get_current_mode() == 3) {
            size_t bytes_read = 0;
            uint8_t* ptr = (uint8_t*)frame_buf;
            
            while (bytes_read < target_bytes && running && get_current_mode() == 3) {
                size_t r = fread(ptr + bytes_read, 1, target_bytes - bytes_read, pipe);
                if (r <= 0) break;
                bytes_read += r;
            }
            
            if (bytes_read < target_bytes) {
                printf("[Video Engine] Pipeline stream ended / disconnected.\n");
                break;
            }
            
            // Frame successfully decoded: copy to global buffer
            pthread_mutex_lock(&video_mutex);
            memcpy(video_frame, frame_buf, sizeof(frame_buf));
            video_updated = true;
            pthread_mutex_unlock(&video_mutex);
        }
        
        if (pipe) {
            pclose(pipe);
            system("pkill -P $(pgrep -o deskclock) yt-dlp 2>/dev/null; pkill -P $(pgrep -o deskclock) ffmpeg 2>/dev/null");
        }
        
        if (running && get_current_mode() == 3) {
            printf("[Video Engine] Reconnecting stream in 2 seconds...\n");
            sleep(2);
        }
    }
    
    return NULL;
}

void video_init(void) {
    running = true;
    if (pthread_create(&video_thread, NULL, video_loop, NULL) != 0) {
        perror("Failed to create video thread");
    }
}

void video_shutdown(void) {
    running = false;
    // Kill any lingering pipeline processes spawned by this process
    system("pkill -P $(pgrep -o deskclock) 2> /dev/null");
    pthread_join(video_thread, NULL);
}
