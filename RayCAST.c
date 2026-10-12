
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>

#include "I2C.h"
#include "OLED.h"

#define W 128
#define H 64

#define TILE 8
#define MAP_W 16
#define MAP_H 8

#define RAY_COUNT 32
#define RAY_STEP 2
#define MAX_RAY_STEPS 160

typedef struct {
    int16_t x;
    int16_t y;
} V2;

/* 1 = wall, 0 = empty */
static const uint8_t world[MAP_H][MAP_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,1,1,0,0,0,1,1,0,0,0,0,1},
    {1,0,0,0,1,1,0,0,0,1,1,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1},
    {1,0,1,1,0,0,0,1,0,0,0,0,1,1,0,1},
    {1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

/* Directions approximately on a circle, scaled by 1000. */
static const int16_t ray_dx[RAY_COUNT] = {
    1000, 981, 924, 831, 707, 556, 383, 195,
    0, -195, -383, -556, -707, -831, -924, -981,
    -1000, -981, -924, -831, -707, -556, -383, -195,
    0, 195, 383, 556, 707, 831, 924, 981
};

static const int16_t ray_dy[RAY_COUNT] = {
    0, 195, 383, 556, 707, 831, 924, 981,
    1000, 981, 924, 831, 707, 556, 383, 195,
    0, -195, -383, -556, -707, -831, -924, -981,
    -1000, -981, -924, -831, -707, -556, -383, -195
};

static V2 light = {16, 32};

/* -------------------------------------------------- */
/* Pixel-coordinate wall query                         */
/* -------------------------------------------------- */

static uint8_t is_wall_at_pixel(int x, int y)
{
    if (x < 0 || x >= W || y < 0 || y >= H)
        return 1;

    int tx = x / TILE;
    int ty = y / TILE;

    return world[ty][tx] != 0;
}

/* -------------------------------------------------- */
/* Bresenham line drawing                              */
/* -------------------------------------------------- */

static void line_bresenham(V2 a, V2 b)
{
    int dx = abs(b.x - a.x);
    int sx = (a.x < b.x) ? 1 : -1;

    int dy = -abs(b.y - a.y);
    int sy = (a.y < b.y) ? 1 : -1;

    int err = dx + dy;

    while (1) {
        if (a.x >= 0 && a.x < W &&
            a.y >= 0 && a.y < H) {
            oled_pixel(a.x, a.y);
        }

        if (a.x == b.x && a.y == b.y)
            break;

        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            a.x += sx;
        }

        if (e2 <= dx) {
            err += dx;
            a.y += sy;
        }
    }
}

/* -------------------------------------------------- */
/* Draw all solid map tiles                            */
/* -------------------------------------------------- */

static void draw_world(void)
{
    for (int ty = 0; ty < MAP_H; ty++) {
        for (int tx = 0; tx < MAP_W; tx++) {
            if (!world[ty][tx])
                continue;

            int x0 = tx * TILE;
            int y0 = ty * TILE;

            for (int y = y0; y < y0 + TILE; y++) {
                for (int x = x0; x < x0 + TILE; x++) {
                    oled_pixel(x, y);
                }
            }
        }
    }
}

/* -------------------------------------------------- */
/* Cast one ray through the tile map                   */
/* -------------------------------------------------- */

static V2 cast_ray(int dx, int dy)
{
    V2 last = light;

    for (int d = 1; d <= MAX_RAY_STEPS; d += RAY_STEP) {
        int x = light.x + (int32_t)dx * d / 1000;
        int y = light.y + (int32_t)dy * d / 1000;

        if (x < 0 || x >= W || y < 0 || y >= H)
            break;

        if (is_wall_at_pixel(x, y))
            return last;

        last.x = x;
        last.y = y;
    }

    return last;
}

/* -------------------------------------------------- */
/* Draw the light-source marker                        */
/* -------------------------------------------------- */

static void draw_light(void)
{
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int x = light.x + dx;
            int y = light.y + dy;

            if (x >= 0 && x < W && y >= 0 && y < H)
                oled_pixel(x, y);
        }
    }
}

/* -------------------------------------------------- */
/* Main: animate rays frame by frame                   */
/* -------------------------------------------------- */

int main(void)
{
    i2c_init();
    oled_init();

    while (1) {
        /* Show the map before the rays start. */
        oled_clear();
        draw_world();
        draw_light();
        oled_update();

        _delay_ms(700);

        /* Reveal one additional ray on each frame. */
        for (int r = 0; r < RAY_COUNT; r++) {
            V2 endpoint = cast_ray(ray_dx[r], ray_dy[r]);

            line_bresenham(light, endpoint);

            /* Re-render walls so the map stays visible. */
            draw_world();
            draw_light();

            oled_update();
            _delay_ms(80);
        }

        _delay_ms(1000);
    }
}
