#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <curl/curl.h>
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
                // Decode the JPEG in memory
                int width, height, channels;
                unsigned char *img_data = stbi_load_from_memory((unsigned char*)chunk.memory, chunk.size, &width, &height, &channels, 3);
                
                if (img_data) {
                    // Stretch image to 160x128 using high-quality downsampling (stb_image_resize2)
                    unsigned char *resized_data = malloc(160 * 128 * 3);
                    if (resized_data) {
                        stbir_resize_uint8_linear(img_data, width, height, 0,
                                                  resized_data, 160, 128, 0,
                                                  STBIR_RGB);
                        
                        pthread_mutex_lock(&camera_mutex);
                        for (int y = 0; y < 128; y++) {
                            for (int x = 0; x < 160; x++) {
                                int idx = (y * 160 + x) * 3;
                                uint8_t r = resized_data[idx];
                                uint8_t g = resized_data[idx + 1];
                                uint8_t b = resized_data[idx + 2];
                                
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
