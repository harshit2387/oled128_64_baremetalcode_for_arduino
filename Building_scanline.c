#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>

#include "I2C.h"
#include "OLED.h"

#define W 128
#define H 64

#define FOCAL 70
#define CENTER_X 64
#define CENTER_Y 32

#define BUILDINGS 10
#define MAX_VERTICES 8

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} V3;

typedef struct
{
    int16_t x;
    int16_t y;
} V2;

typedef struct
{
    int16_t x;
    int16_t z;
    int16_t width;
    int16_t height;
} Building;

Building buildings[BUILDINGS] =
{
    {-58,  35, 18, 30},
    { 55,  45, 20, 35},
    {-52,  70, 25, 42},
    { 50,  80, 18, 28},
    {-60, 105, 20, 50},
    { 58, 115, 26, 38},
    {-48, 140, 30, 35},
    { 48, 150, 20, 55},
    {-65, 180, 25, 45},
    { 62, 195, 30, 35}
};

void line_bresenham_algo(int x1, int y1, int x2, int y2)
{
    int x0 = x1;
    int y0 = y1;

    int dx = abs(x2 - x1);
    int sx = (x1 < x2) ? 1 : -1;

    int dy = -abs(y2 - y1);
    int sy = (y1 < y2) ? 1 : -1;

    int err = dx + dy;

    while (1)
    {
        if (x0 >= 0 && x0 < W && y0 >= 0 && y0 < H)
        {
            oled_pixel(x0, y0);
        }

        if (x0 == x2 && y0 == y2)
        {
            break;
        }

        int e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

V2 project(V3 p)
{
    V2 result;

    if (p.z < 5)
    {
        p.z = 5;
    }

    result.x = CENTER_X + ((int32_t)p.x * FOCAL) / p.z;
    result.y = CENTER_Y - ((int32_t)p.y * FOCAL) / p.z;

    return result;
}

void scanline_fill(V2 vertices[], uint8_t count, uint8_t pattern)
{
    if (count < 3 || count > MAX_VERTICES)
    {
        return;
    }

    int16_t minY = vertices[0].y;
    int16_t maxY = vertices[0].y;

    for (uint8_t i = 1; i < count; i++)
    {
        if (vertices[i].y < minY)
        {
            minY = vertices[i].y;
        }

        if (vertices[i].y > maxY)
        {
            maxY = vertices[i].y;
        }
    }

    if (minY < 0)
    {
        minY = 0;
    }

    if (maxY >= H)
    {
        maxY = H - 1;
    }

    int16_t intersections[MAX_VERTICES];

    for (int16_t y = minY; y <= maxY; y++)
    {
        uint8_t n = 0;

        for (uint8_t i = 0; i < count; i++)
        {
            V2 a = vertices[i];
            V2 b = vertices[(i + 1) % count];

            if (a.y == b.y)
            {
                continue;
            }

            int16_t lowerY = (a.y < b.y) ? a.y : b.y;
            int16_t upperY = (a.y > b.y) ? a.y : b.y;

            if (y >= lowerY && y < upperY)
            {
                int32_t numerator =
                    (int32_t)(y - a.y) * (b.x - a.x);

                int16_t x =
                    a.x + numerator / (b.y - a.y);

                if (n < MAX_VERTICES)
                {
                    intersections[n++] = x;
                }
            }
        }

        for (uint8_t i = 1; i < n; i++)
        {
            int16_t key = intersections[i];
            int8_t j = (int8_t)i - 1;

            while (j >= 0 && intersections[j] > key)
            {
                intersections[j + 1] = intersections[j];
                j--;
            }

            intersections[j + 1] = key;
        }

        for (uint8_t i = 0; i + 1 < n; i += 2)
        {
            int16_t xStart = intersections[i];
            int16_t xEnd = intersections[i + 1];

            if (xStart < 0)
            {
                xStart = 0;
            }

            if (xEnd >= W)
            {
                xEnd = W - 1;
            }

            for (int16_t x = xStart; x <= xEnd; x++)
            {
                uint8_t draw = 0;

                if (pattern == 0)
                {
                    draw = (((x + y) & 1) == 0);
                }
                else if (pattern == 1)
                {
                    draw = ((x & 1) == 0);
                }
                else
                {
                    draw = ((y & 1) == 0);
                }

                if (draw)
                {
                    oled_pixel(x, y);
                }
            }
        }
    }
}

void draw_building(Building b)
{
    int16_t halfWidth = b.width / 2;

    int16_t ground = -18;
    int16_t top = ground + b.height;

    V3 p[8];
    V2 s[8];

    p[0] = (V3){b.x - halfWidth, ground, b.z};
    p[1] = (V3){b.x + halfWidth, ground, b.z};
    p[2] = (V3){b.x + halfWidth, top, b.z};
    p[3] = (V3){b.x - halfWidth, top, b.z};

    p[4] = (V3){b.x - halfWidth, ground, b.z + 15};
    p[5] = (V3){b.x + halfWidth, ground, b.z + 15};
    p[6] = (V3){b.x + halfWidth, top, b.z + 15};
    p[7] = (V3){b.x - halfWidth, top, b.z + 15};

    for (uint8_t i = 0; i < 8; i++)
    {
        s[i] = project(p[i]);
    }

    V2 front[4] = {s[0], s[1], s[2], s[3]};
    V2 side[4]  = {s[1], s[5], s[6], s[2]};
    V2 roof[4]  = {s[3], s[2], s[6], s[7]};

    scanline_fill(side, 4, 1);
    scanline_fill(roof, 4, 2);
    scanline_fill(front, 4, 0);

    for (uint8_t i = 0; i < 4; i++)
    {
        uint8_t j = (i + 1) % 4;

        line_bresenham_algo(
            s[i].x, s[i].y,
            s[j].x, s[j].y
        );

        line_bresenham_algo(
            s[i + 4].x, s[i + 4].y,
            s[j + 4].x, s[j + 4].y
        );

        line_bresenham_algo(
            s[i].x, s[i].y,
            s[i + 4].x, s[i + 4].y
        );
    }
}

void draw_road(void)
{
    V3 leftNear  = {-55, -18, 25};
    V3 rightNear = { 55, -18, 25};
    V3 leftFar   = { -8, -18, 220};
    V3 rightFar  = {  8, -18, 220};

    V2 ln = project(leftNear);
    V2 rn = project(rightNear);
    V2 lf = project(leftFar);
    V2 rf = project(rightFar);

    line_bresenham_algo(ln.x, ln.y, lf.x, lf.y);
    line_bresenham_algo(rn.x, rn.y, rf.x, rf.y);
    line_bresenham_algo(lf.x, lf.y, rf.x, rf.y);
}

void draw_lane_lines(int16_t roadOffset)
{
    for (int16_t z = 30; z < 220; z += 35)
    {
        V3 left  = {-4, -17, z + roadOffset};
        V3 right = { 4, -17, z + roadOffset};

        if (left.z > 20 && left.z < 250)
        {
            V2 l = project(left);
            V2 r = project(right);

            line_bresenham_algo(l.x, l.y, r.x, r.y);
        }
    }
}

void draw_horizon(void)
{
    line_bresenham_algo(0, 35, 127, 35);
}

int main(void)
{
    i2c_init();
    oled_init();

    int16_t roadOffset = 0;

    while (1)
    {
        oled_clear();

        draw_horizon();
        draw_road();
        draw_lane_lines(roadOffset);

        for (uint8_t i = 0; i < BUILDINGS; i++)
        {
            draw_building(buildings[i]);
        }

        oled_update();

        roadOffset += 4;

        if (roadOffset >= 35)
        {
            roadOffset = 0;
        }

        for (uint8_t i = 0; i < BUILDINGS; i++)
        {
            buildings[i].z -= 2;

            if (buildings[i].z < 20)
            {
                buildings[i].z = 200;
            }
        }

        _delay_ms(50);
    }

    return 0;
}
