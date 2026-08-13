#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <curl/curl.h>
#include "weather.h"

WeatherData g_weather_data;
static pthread_t weather_thread;
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

static void parse_weather_json(const char* json) {
    char temp_c[16] = {0};
    char desc[64] = {0};
    char hum[16] = {0};

    const char* temp_ptr = strstr(json, "\"temp_C\": \"");
    if (temp_ptr) {
        temp_ptr += 11;
        int i = 0;
        while (temp_ptr[i] != '"' && temp_ptr[i] != '\0' && i < 15) {
            temp_c[i] = temp_ptr[i];
            i++;
        }
        temp_c[i] = '\0';
    }

    const char* desc_ptr = strstr(json, "\"weatherDesc\": [{\"value\": \"");
    if (!desc_ptr) {
        // Fallback for slight JSON variations
        desc_ptr = strstr(json, "\"value\": \"");
    } else {
        desc_ptr += 27;
    }
    
    if (desc_ptr) {
        // If we matched the fallback
        if (desc_ptr == strstr(json, "\"value\": \"")) {
            desc_ptr += 10;
        }
        int i = 0;
        while (desc_ptr[i] != '"' && desc_ptr[i] != '\0' && i < 63) {
            desc[i] = desc_ptr[i];
            i++;
        }
        desc[i] = '\0';
    }

    const char* hum_ptr = strstr(json, "\"humidity\": \"");
    if (hum_ptr) {
        hum_ptr += 13;
        int i = 0;
        while (hum_ptr[i] != '"' && hum_ptr[i] != '\0' && i < 15) {
            hum[i] = hum_ptr[i];
            i++;
        }
        hum[i] = '\0';
    }

    pthread_mutex_lock(&g_weather_data.mutex);
    if (temp_c[0] != '\0') snprintf(g_weather_data.temp, sizeof(g_weather_data.temp), "%sC", temp_c);
    if (desc[0] != '\0') snprintf(g_weather_data.desc, sizeof(g_weather_data.desc), "%s", desc);
    if (hum[0] != '\0') snprintf(g_weather_data.hum, sizeof(g_weather_data.hum), "%s%%", hum);
    g_weather_data.updated = true;
    pthread_mutex_unlock(&g_weather_data.mutex);
}

static void* weather_loop(void* arg) {
    CURL *curl_handle;
    CURLcode res;
    struct MemoryStruct chunk;

    while (running) {
        printf("Fetching live weather data...\n");
        chunk.memory = malloc(1);
        chunk.size = 0;

        curl_global_init(CURL_GLOBAL_ALL);
        curl_handle = curl_easy_init();
        
        if (curl_handle) {
            curl_easy_setopt(curl_handle, CURLOPT_URL, "http://wttr.in/Vancouver?format=j1");
            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
            curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "curl/7.68.0");
            curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);

            res = curl_easy_perform(curl_handle);
            if (res == CURLE_OK) {
                parse_weather_json(chunk.memory);
            } else {
                printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            }
            curl_easy_cleanup(curl_handle);
        }
        free(chunk.memory);
        curl_global_cleanup();

        // Wait 10 minutes between fetches (600 seconds)
        for (int i = 0; i < 600; i++) {
            if (!running) break;
            sleep(1);
        }
    }
    return NULL;
}

void weather_init(void) {
    pthread_mutex_init(&g_weather_data.mutex, NULL);
    strcpy(g_weather_data.temp, "--C");
    strcpy(g_weather_data.desc, "FETCHING...");
    strcpy(g_weather_data.hum, "--%");
    g_weather_data.updated = true;
    running = true;
    pthread_create(&weather_thread, NULL, weather_loop, NULL);
}

void weather_shutdown(void) {
    running = false;
    pthread_join(weather_thread, NULL);
    pthread_mutex_destroy(&g_weather_data.mutex);
}
