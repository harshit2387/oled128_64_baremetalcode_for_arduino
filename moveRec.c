#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>

#include "I2C.h"
#include "OLED.h"

#define W 128
#define H 64

typedef struct {
    int16_t x, y;
} V2;

V2 p[8] = {
    {40,20}, {88,20},
    {88,44}, {40,44},

    {68,46}, {100,46},
    {100,58}, {68,58}
};

uint8_t edges[12][2] = {
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7}
};

void line(int16_t x0, int16_t y0,
          int16_t x1, int16_t y1)
{
    int16_t dx = abs(x1-x0);
    int16_t sx = x0 < x1 ? 1 : -1;

    int16_t dy = -abs(y1-y0);
    int16_t sy = y0 < y1 ? 1 : -1;

    int16_t err = dx + dy;

    while (1)
    {
        if (x0 >= 0 && x0 < W &&
            y0 >= 0 && y0 < H)
            oled_pixel(x0, y0);

        if (x0 == x1 && y0 == y1)
            break;

        int16_t e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

int main(void)
{
    i2c_init();
    oled_init();

    int16_t offsetX = 0;
    int8_t direction = 1;

    while (1)
    {
        oled_clear();

        /* Front rectangle */
        for (uint8_t i = 0; i < 4; i++)
        {
            line(
                p[edges[i][0]].x,
                p[edges[i][0]].y,
                p[edges[i][1]].x,
                p[edges[i][1]].y
            );
        }

        /* Back rectangle */
        for (uint8_t i = 4; i < 8; i++)
        {
            line(
                p[edges[i][0]].x + offsetX,
                p[edges[i][0]].y,
                p[edges[i][1]].x + offsetX,
                p[edges[i][1]].y
            );
        }

        /* Connecting lines */
        for (uint8_t i = 8; i < 12; i++)
        {
            line(
                p[edges[i][0]].x,
                p[edges[i][0]].y,
                p[edges[i][1]].x + offsetX,
                p[edges[i][1]].y
            );
        }

        oled_update();

        /* Move left/right */
        offsetX += direction;

        if (offsetX >= 20)
            direction = -1;

        if (offsetX <= -100)
            direction = 1;

        _delay_ms(40);
    }
}
