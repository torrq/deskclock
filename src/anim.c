#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "anim.h"
#include "tft.h"

static long tick = 0;

typedef struct {
    int x;
    int y;
    int speed;
} Particle;

typedef struct {
    int x;
    int y;
} Star;

typedef struct {
    float x;
    float y;
    float dx;
    float dy;
    int life;
    bool active;
} Comet;

static Particle drops[8];
static Particle flakes[4];
static Star stars[30];
static Comet comet = {0};

static const int drop_pixels[][2] = {
    {0,0}, {0,1}, 
    {-1,2}, {0,2}, {1,2}, 
    {-2,3}, {-1,3}, {0,3}, {1,3}, {2,3}, 
    {-2,4}, {-1,4}, {0,4}, {1,4}, {2,4}, 
    {-1,5}, {0,5}, {1,5}
};
static const int num_drop_pixels = 18;

typedef struct {
    int x; int y;
} Point;

static Point large_flake_pixels[100];
static int num_flake_pixels = 0;

static int random_x(void) {
    return 2 + (rand() % 36);
}

void anim_init(void) {
    for (int i = 0; i < 8; i++) {
        drops[i].x = random_x();
        drops[i].y = i * (128 / 8);
        drops[i].speed = 10;
    }
    for (int i = 0; i < 4; i++) {
        flakes[i].x = random_x();
        flakes[i].y = i * (128 / 4);
        flakes[i].speed = 1 + (rand() % 2);
    }
    for (int i = 0; i < 30; i++) {
        stars[i].x = rand() % 160;
        stars[i].y = rand() % 128;
    }
    
    // Generate snowflake pixels like Python
    int radius = 8;
    for (int i = 0; i < 8; i++) {
        float angle = i * 45 * 3.14159f / 180.0;
        for (int r = 0; r < radius; r++) {
            large_flake_pixels[num_flake_pixels].x = (int)(cos(angle) * r);
            large_flake_pixels[num_flake_pixels].y = (int)(sin(angle) * r);
            num_flake_pixels++;
        }
    }
    for (int i = 0; i < 4; i++) {
        float angle = i * 90 * 3.14159f / 180.0;
        int fork_r = 4;
        int fx = (int)(cos(angle) * fork_r);
        int fy = (int)(sin(angle) * fork_r);
        float left_angle = angle + 45 * 3.14159f / 180.0;
        float right_angle = angle - 45 * 3.14159f / 180.0;
        for (int d = 1; d < 4; d++) {
            large_flake_pixels[num_flake_pixels].x = (int)(fx + cos(left_angle)*d);
            large_flake_pixels[num_flake_pixels].y = (int)(fy + sin(left_angle)*d);
            num_flake_pixels++;
            large_flake_pixels[num_flake_pixels].x = (int)(fx + cos(right_angle)*d);
            large_flake_pixels[num_flake_pixels].y = (int)(fy + sin(right_angle)*d);
            num_flake_pixels++;
        }
    }
}

void anim_draw(const char* desc, bool is_night) {
    tick++;
    
    if (strstr(desc, "FETCHING")) {
        tft_fill(0x0000);
        return;
    }
    
    if (strstr(desc, "SUN") || strstr(desc, "CLEAR")) {
        tft_fill(0x0000);
        if (is_night) {
            for (int i = 0; i < 30; i++) {
                if ((tick + i * 7) % 15 < 3) {
                    tft_draw_pixel(stars[i].x, stars[i].y, 0xFFFF);
                } else {
                    tft_draw_pixel(stars[i].x, stars[i].y, 0x8410);
                }
            }
            
            // Moon disabled by request
            
            if (comet.active) {
                int sx = (int)comet.x;
                int sy = (int)comet.y;
                
                tft_draw_pixel(sx, sy, 0xFFFF);
                tft_draw_pixel(sx+1, sy, 0xFFE0);
                tft_draw_pixel(sx, sy+1, 0xFFE0);
                tft_draw_pixel(sx+1, sy+1, 0xFFFF);
                
                for (int d = 1; d < 7; d++) {
                    int tx = sx - (int)(comet.dx * d * 0.5);
                    int ty = sy - (int)(comet.dy * d * 0.5);
                    uint16_t color = (d < 4) ? 0xFD20 : 0xF800;
                    tft_draw_pixel(tx, ty, color);
                    if (d < 4) {
                        tft_draw_pixel(tx+1, ty, color);
                        tft_draw_pixel(tx, ty+1, color);
                    }
                }
                
                comet.x += comet.dx;
                comet.y += comet.dy;
                comet.life--;
                
                if (comet.life <= 0 || sx < 0 || sx > 159 || sy > 127) {
                    comet.active = false;
                }
            } else {
                if ((rand() % 1000) < 10) { 
                    comet.active = true;
                    comet.x = 120 + (rand() % 40);
                    comet.y = rand() % 21;
                    comet.dx = -2.0 - ((rand() % 200) / 100.0);
                    comet.dy = 1.0 + ((rand() % 200) / 100.0);
                    comet.life = 80;
                }
            }
            
        } else {
            int cx = 80, cy = 64;
            int radius = 45 + (int)(sin(tick * 0.05) * 8);
            int r_sq = radius * radius;
            int r1_sq = (int)((radius * 0.4) * (radius * 0.4));
            int r2_sq = (int)((radius * 0.7) * (radius * 0.7));
            
            for (int y = cy - radius; y < cy + radius; y++) {
                for (int x = cx - radius; x < cx + radius; x++) {
                    int dist_sq = (x - cx)*(x - cx) + (y - cy)*(y - cy);
                    if (dist_sq <= r_sq) {
                        uint16_t color;
                        if (dist_sq < r1_sq) color = 0xFFFF;
                        else if (dist_sq < r2_sq) color = 0xFFE0;
                        else color = 0xFD20;
                        tft_draw_pixel(x, y, color);
                    }
                }
            }
            
            for (int i = 0; i < 12; i++) {
                float angle = (i * 30 + tick * 0.5) * 3.14159f / 180.0;
                for (int r = radius + 5; r < radius + 35; r++) {
                    int rx = cx + (int)(cos(angle) * r);
                    int ry = cy + (int)(sin(angle) * r);
                    tft_draw_pixel(rx, ry, 0xFD20);
                    tft_draw_pixel(rx+1, ry, 0xFD20);
                    tft_draw_pixel(rx, ry+1, 0xFD20);
                    tft_draw_pixel(rx+1, ry+1, 0xFD20);
                }
            }
        }
    } else if (strstr(desc, "RAIN") || strstr(desc, "DRIZZLE") || strstr(desc, "SHOWER")) {
        tft_fill(0x0000);
        for (int i = 0; i < 8; i++) {
            for (int p = 0; p < num_drop_pixels; p++) {
                tft_draw_pixel(drops[i].x + drop_pixels[p][0], drops[i].y + drop_pixels[p][1], 0x03FF);
            }
            
            drops[i].y += 2; // Slower rain
            if (drops[i].y > 128) {
                drops[i].y = -5;
                drops[i].x = random_x();
            }
        }
    } else if (strstr(desc, "SNOW") || strstr(desc, "ICE") || strstr(desc, "BLIZZARD")) {
        tft_fill(0x0000);
        for (int i = 0; i < 4; i++) {
            int fx = flakes[i].x;
            int fy = flakes[i].y;
            int speed = flakes[i].speed;
            
            for (int p = 0; p < num_flake_pixels; p++) {
                tft_draw_pixel(fx + large_flake_pixels[p].x, fy + large_flake_pixels[p].y, 0xFFFF);
            }
            
            if (tick % (3 - speed) == 0) {
                flakes[i].y += 1;
            }
            
            flakes[i].x += (int)(sin(tick * 0.1 + i));
            
            if (flakes[i].y > 128 + 10) {
                flakes[i].y = -10;
                flakes[i].x = random_x();
            }
        }
    } else { // CLOUD
        tft_fill(0x0000);
        int cloud_offset = (int)(sin(tick * 0.1) * 10);
        
        int circles[5][3] = {
            {80, 80, 45}, {40, 90, 35}, {120, 90, 35}, {55, 65, 40}, {105, 65, 35}
        };
        
        for (int i = 0; i < 5; i++) {
            int cx = circles[i][0];
            int cy = circles[i][1] + cloud_offset;
            int cr = circles[i][2];
            
            for (int y = cy - cr; y < cy + cr; y++) {
                for (int x = cx - cr; x < cx + cr; x++) {
                    if ((x - cx)*(x - cx) + (y - cy)*(y - cy) <= cr*cr) {
                        if ((x + y) % 2 == 0) {
                            tft_draw_pixel(x, y, 0x7BEF);
                        }
                    }
                }
            }
        }
    }
}
