#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <curl/curl.h>
#include "camera.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
        chunk.memory = malloc(1);
        chunk.size = 0;

        curl_global_init(CURL_GLOBAL_ALL);
        curl_handle = curl_easy_init();
        
        if (curl_handle) {
            curl_easy_setopt(curl_handle, CURLOPT_URL, "https://trafficcams.vancouver.ca/cameraimages/BurrardCanadaWest.jpg");
            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
            curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "curl/7.68.0");
            curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 15L);

            res = curl_easy_perform(curl_handle);
            if (res == CURLE_OK && chunk.size > 0) {
                // Decode the JPEG in memory
                int width, height, channels;
                unsigned char *img_data = stbi_load_from_memory((unsigned char*)chunk.memory, chunk.size, &width, &height, &channels, 3);
                
                if (img_data) {
                    pthread_mutex_lock(&camera_mutex);
                    // Stretch image to 160x128 using Bilinear Interpolation
                    for (int y = 0; y < 128; y++) {
                        for (int x = 0; x < 160; x++) {
                            float src_x = ((float)x / 159.0f) * (width - 1);
                            float src_y = ((float)y / 127.0f) * (height - 1);
                            
                            int x1 = (int)src_x;
                            int y1 = (int)src_y;
                            int x2 = x1 + 1;
                            int y2 = y1 + 1;
                            if (x2 >= width) x2 = width - 1;
                            if (y2 >= height) y2 = height - 1;
                            
                            float dx = src_x - x1;
                            float dy = src_y - y1;
                            
                            int idx11 = (y1 * width + x1) * 3;
                            int idx21 = (y1 * width + x2) * 3;
                            int idx12 = (y2 * width + x1) * 3;
                            int idx22 = (y2 * width + x2) * 3;
                            
                            uint8_t r = (1.0f - dx) * (1.0f - dy) * img_data[idx11 + 0] +
                                        dx * (1.0f - dy) * img_data[idx21 + 0] +
                                        (1.0f - dx) * dy * img_data[idx12 + 0] +
                                        dx * dy * img_data[idx22 + 0];
                                        
                            uint8_t g = (1.0f - dx) * (1.0f - dy) * img_data[idx11 + 1] +
                                        dx * (1.0f - dy) * img_data[idx21 + 1] +
                                        (1.0f - dx) * dy * img_data[idx12 + 1] +
                                        dx * dy * img_data[idx22 + 1];
                                        
                            uint8_t b = (1.0f - dx) * (1.0f - dy) * img_data[idx11 + 2] +
                                        dx * (1.0f - dy) * img_data[idx21 + 2] +
                                        (1.0f - dx) * dy * img_data[idx12 + 2] +
                                        dx * dy * img_data[idx22 + 2];
                            
                            // Convert to RGB565
                            uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                            
                            camera_frame[y * 160 + x] = rgb565;
                        }
                    }
                    camera_updated = true;
                    pthread_mutex_unlock(&camera_mutex);
                    stbi_image_free(img_data);
                    printf("Camera image updated.\n");
                } else {
                    printf("Failed to decode camera JPEG.\n");
                }
            } else {
                printf("curl_easy_perform() failed for camera: %s\n", curl_easy_strerror(res));
            }
            curl_easy_cleanup(curl_handle);
        }
        free(chunk.memory);
        curl_global_cleanup();

        // Wait 5 minutes between fetches (300 seconds)
        for (int i = 0; i < 300; i++) {
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
