#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <curl/curl.h>
#include <math.h>
#include "camera.h"
#include "config.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

uint16_t camera_frame[160 * 128];
pthread_mutex_t camera_mutex;
bool camera_updated = false;

static pthread_t camera_thread;
static bool running = false;

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static void* camera_loop(void* arg) {
    CURL *curl_handle;
    CURLcode res;
    struct MemoryStruct chunk;

    while (running) {
        printf("Fetching live camera data...\n");
        unsigned char *img_data = NULL;
        int width, height, channels;

        if (strstr(g_camera_url, "youtube.com") || strstr(g_camera_url, "youtu.be")) { // YouTube Mode
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "ffmpeg -y -i $(yt-dlp -g -f b --no-warnings \"%s\") -vframes 1 -q:v 2 /tmp/deskclock_camera.jpg > /dev/null 2>&1", g_camera_url);
            printf("Executing YouTube command for %s\n", g_camera_url);
            int ret = system(cmd);
            if (ret == 0) {
                img_data = stbi_load("/tmp/deskclock_camera.jpg", &width, &height, &channels, 3);
            } else {
                printf("YouTube fetch failed with return code %d\n", ret);
            }
        } else { // Direct URL Mode
            chunk.memory = malloc(1);
            chunk.size = 0;

            curl_global_init(CURL_GLOBAL_ALL);
            curl_handle = curl_easy_init();
            
            if (curl_handle) {
                curl_easy_setopt(curl_handle, CURLOPT_URL, g_camera_url);
                curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
                curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
                curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "curl/7.68.0");
                curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 15L);

                res = curl_easy_perform(curl_handle);
                if (res == CURLE_OK && chunk.size > 0) {
                    img_data = stbi_load_from_memory((unsigned char*)chunk.memory, chunk.size, &width, &height, &channels, 3);
                } else {
                    printf("curl_easy_perform() failed for camera: %s\n", curl_easy_strerror(res));
                }
                curl_easy_cleanup(curl_handle);
            }
            free(chunk.memory);
            curl_global_cleanup();
        }

        if (img_data) {
            // Stretch image to 160x128 using high-quality downsampling (stb_image_resize2)
            // We must use _srgb because JPEGs are sRGB, and linear downsampling washes out the image
            unsigned char *resized_data = malloc(160 * 128 * 3);
            if (resized_data) {
                int c_top = g_camera_crop_top;
                int c_bottom = g_camera_crop_bottom;
                int c_left = g_camera_crop_left;
                int c_right = g_camera_crop_right;
                
                // Sanity check crops
                if (c_top + c_bottom >= height) { c_top = 0; c_bottom = 0; }
                if (c_left + c_right >= width) { c_left = 0; c_right = 0; }
                
                int src_w = width - c_left - c_right;
                int src_h = height - c_top - c_bottom;
                int src_x = c_left;
                int src_y = c_top;
                
                // Crop-to-fit (object-fit: cover)
                if (g_camera_scale_mode == 1) {
                    float src_aspect = (float)src_w / (float)src_h;
                    float dst_aspect = 160.0f / 128.0f;
                    
                    if (src_aspect > dst_aspect) {
                        int new_w = (int)(src_h * dst_aspect);
                        src_x += (src_w - new_w) / 2;
                        src_w = new_w;
                    } else {
                        int new_h = (int)(src_w / dst_aspect);
                        src_y += (src_h - new_h) / 2;
                        src_h = new_h;
                    }
                }
                
                unsigned char *src_ptr = img_data + (src_y * width + src_x) * 3;
                
                stbir_resize_uint8_srgb(src_ptr, src_w, src_h, width * 3,
                                        resized_data, 160, 128, 0,
                                        STBIR_RGB);
                
                pthread_mutex_lock(&camera_mutex);
                
                uint8_t gamma_lut[256];
                for (int i = 0; i < 256; i++) {
                    float v = (float)i / 255.0f;
                    gamma_lut[i] = (uint8_t)(powf(v, g_camera_gamma) * 255.0f + 0.5f);
                }
                
                for (int y = 0; y < 128; y++) {
                    for (int x = 0; x < 160; x++) {
                        int idx = (y * 160 + x) * 3;
                        uint8_t r = gamma_lut[resized_data[idx]];
                        uint8_t g = gamma_lut[resized_data[idx + 1]];
                        uint8_t b = gamma_lut[resized_data[idx + 2]];
                        
                        // Convert to RGB565
                        camera_frame[y * 160 + x] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                    }
                }
                camera_updated = true;
                pthread_mutex_unlock(&camera_mutex);
                free(resized_data);
                printf("Camera image updated.\n");
            } else {
                printf("Failed to allocate memory for resized image.\n");
            }
            stbi_image_free(img_data);
        } else {
            printf("Failed to acquire camera image.\n");
        }

        for (int i = 0; i < g_camera_refresh_interval; i++) {
            if (!running) break;
            sleep(1);
        }
    }
    return NULL;
}

void camera_init(void) {
    pthread_mutex_init(&camera_mutex, NULL);
    // Fill with black initially
    for (int i = 0; i < 160 * 128; i++) {
        camera_frame[i] = 0x0000;
    }
    camera_updated = false;
    running = true;
    pthread_create(&camera_thread, NULL, camera_loop, NULL);
}

void camera_shutdown(void) {
    running = false;
    pthread_join(camera_thread, NULL);
    pthread_mutex_destroy(&camera_mutex);
}
