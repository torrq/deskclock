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

static void* video_loop(void* arg) {
    char stream_url[2048] = {0};
    uint16_t frame_buf[VIDEO_FRAME_SIZE];
    
    while (running) {
        stream_url[0] = '\0';
        
        // 1. Resolve live stream URL
        if (strstr(g_camera_url, "youtube.com") || strstr(g_camera_url, "youtu.be")) {
            printf("[Video Engine] Resolving YouTube stream URL with yt-dlp...\n");
            char ytdlp_cmd[512];
            snprintf(ytdlp_cmd, sizeof(ytdlp_cmd), "yt-dlp -g -f worst --no-warnings \"%s\"", g_camera_url);
            
            FILE* pipe = popen(ytdlp_cmd, "r");
            if (pipe) {
                if (fgets(stream_url, sizeof(stream_url), pipe) != NULL) {
                    // Strip trailing newline
                    size_t len = strlen(stream_url);
                    if (len > 0 && stream_url[len - 1] == '\n') {
                        stream_url[len - 1] = '\0';
                    }
                }
                pclose(pipe);
            }
        } else {
            // Direct stream URL
            strncpy(stream_url, g_camera_url, sizeof(stream_url) - 1);
            stream_url[sizeof(stream_url) - 1] = '\0';
        }
        
        if (strlen(stream_url) == 0) {
            printf("[Video Engine] Failed to obtain stream URL. Retrying in 5 seconds...\n");
            for (int i = 0; i < 5 && running; i++) sleep(1);
            continue;
        }
        
        printf("[Video Engine] Starting ffmpeg live stream pipe at %d FPS (Gamma: %.2f)...\n", g_video_fps, g_camera_gamma);
        
        float ffmpeg_gamma = (g_camera_gamma > 0.1f) ? (1.0f / g_camera_gamma) : 0.45f;
        
        char ffmpeg_cmd[3072];
        snprintf(ffmpeg_cmd, sizeof(ffmpeg_cmd),
            "ffmpeg -nostats -loglevel error "
            "-fflags +nobuffer+genpts+discardcorrupt -flags low_delay "
            "-reconnect 1 -reconnect_streamed 1 -reconnect_delay_max 5 "
            "-i \"%s\" -vf \"fps=%d,scale=160:128,eq=gamma=%.3f:contrast=1.15:saturation=1.3\" -f rawvideo -pix_fmt rgb565le -",
            stream_url, g_video_fps, ffmpeg_gamma);
        
        FILE* ffmpeg_pipe = popen(ffmpeg_cmd, "r");
        if (!ffmpeg_pipe) {
            printf("[Video Engine] Failed to start ffmpeg process.\n");
            for (int i = 0; i < 5 && running; i++) sleep(1);
            continue;
        }
        
        // Read raw video frames continuously
        const size_t target_bytes = VIDEO_FRAME_SIZE * sizeof(uint16_t);
        while (running) {
            size_t bytes_read = 0;
            uint8_t* ptr = (uint8_t*)frame_buf;
            
            while (bytes_read < target_bytes && running) {
                size_t r = fread(ptr + bytes_read, 1, target_bytes - bytes_read, ffmpeg_pipe);
                if (r <= 0) break;
                bytes_read += r;
            }
            
            if (bytes_read < target_bytes) {
                // Stream disconnected or URL expired
                printf("[Video Engine] Stream disconnected / EOF reached.\n");
                break;
            }
            
            // Frame successfully decoded: copy to global buffer
            pthread_mutex_lock(&video_mutex);
            memcpy(video_frame, frame_buf, sizeof(frame_buf));
            video_updated = true;
            pthread_mutex_unlock(&video_mutex);
        }
        
        if (ffmpeg_pipe) {
            pclose(ffmpeg_pipe);
        }
        
        if (running) {
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
    // Kill any lingering ffmpeg children spawned by this process
    system("pkill -P $(pgrep -o deskclock) ffmpeg > /dev/null 2>&1");
    pthread_join(video_thread, NULL);
}
