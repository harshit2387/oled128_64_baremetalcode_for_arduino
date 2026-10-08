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

void line_bresenham_algo(
    int x1,
    int y1,
    int x2,
    int y2
)
{
    int x0 = x1;
    int y0 = y1;

    int dx = abs(x2 - x1);
    int sx = x1 < x2 ? 1 : -1;

    int dy = -abs(y2 - y1);
    int sy = y1 < y2 ? 1 : -1;

    int err = dx + dy;

    while (1)
    {
        if (x0 >= 0 && x0 < W &&
            y0 >= 0 && y0 < H)
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

    int16_t projectedX =
        (p.x * FOCAL) / p.z;

    int16_t projectedY =
        (p.y * FOCAL) / p.z;

    result.x =
        CENTER_X + projectedX;

    result.y =
        CENTER_Y - projectedY;

    return result;
}

void draw_building(Building b)
{
    int16_t halfWidth = b.width / 2;

    int16_t ground = -18;
    int16_t top = ground + b.height;

    V3 p[8];
    V2 s[8];

    p[0].x = b.x - halfWidth;
    p[0].y = ground;
    p[0].z = b.z;

    p[1].x = b.x + halfWidth;
    p[1].y = ground;
    p[1].z = b.z;

    p[2].x = b.x + halfWidth;
    p[2].y = top;
    p[2].z = b.z;

    p[3].x = b.x - halfWidth;
    p[3].y = top;
    p[3].z = b.z;

    p[4].x = b.x - halfWidth;
    p[4].y = ground;
    p[4].z = b.z + 15;

    p[5].x = b.x + halfWidth;
    p[5].y = ground;
    p[5].z = b.z + 15;

    p[6].x = b.x + halfWidth;
    p[6].y = top;
    p[6].z = b.z + 15;

    p[7].x = b.x - halfWidth;
    p[7].y = top;
    p[7].z = b.z + 15;

    for (uint8_t i = 0; i < 8; i++)
    {
        s[i] = project(p[i]);
    }

    line_bresenham_algo(
        s[0].x, s[0].y,
        s[1].x, s[1].y
    );

    line_bresenham_algo(
        s[1].x, s[1].y,
        s[2].x, s[2].y
    );

    line_bresenham_algo(
        s[2].x, s[2].y,
        s[3].x, s[3].y
    );

    line_bresenham_algo(
        s[3].x, s[3].y,
        s[0].x, s[0].y
    );

    line_bresenham_algo(
        s[4].x, s[4].y,
        s[5].x, s[5].y
    );

    line_bresenham_algo(
        s[5].x, s[5].y,
        s[6].x, s[6].y
    );

    line_bresenham_algo(
        s[6].x, s[6].y,
        s[7].x, s[7].y
    );

    line_bresenham_algo(
        s[7].x, s[7].y,
        s[4].x, s[4].y
    );

    line_bresenham_algo(
        s[0].x, s[0].y,
        s[4].x, s[4].y
    );

    line_bresenham_algo(
        s[1].x, s[1].y,
        s[5].x, s[5].y
    );

    line_bresenham_algo(
        s[2].x, s[2].y,
        s[6].x, s[6].y
    );

    line_bresenham_algo(
        s[3].x, s[3].y,
        s[7].x, s[7].y
    );
}

void draw_road(void)
{
    V3 leftNear;
    V3 rightNear;
    V3 leftFar;
    V3 rightFar;

    leftNear.x = -55;
    leftNear.y = -18;
    leftNear.z = 25;

    rightNear.x = 55;
    rightNear.y = -18;
    rightNear.z = 25;

    leftFar.x = -8;
    leftFar.y = -18;
    leftFar.z = 220;

    rightFar.x = 8;
    rightFar.y = -18;
    rightFar.z = 220;

    V2 ln = project(leftNear);
    V2 rn = project(rightNear);

    V2 lf = project(leftFar);
    V2 rf = project(rightFar);

    line_bresenham_algo(
        ln.x, ln.y,
        lf.x, lf.y
    );

    line_bresenham_algo(
        rn.x, rn.y,
        rf.x, rf.y
    );

    line_bresenham_algo(
        lf.x, lf.y,
        rf.x, rf.y
    );
}

void draw_lane_lines(int16_t roadOffset)
{
    int16_t z;

    for (z = 30; z < 220; z += 35)
    {
        V3 left;
        V3 right;

        left.x = -4;
        left.y = -17;
        left.z = z + roadOffset;

        right.x = 4;
        right.y = -17;
        right.z = z + roadOffset;

        if (left.z > 20 && left.z < 250)
        {
            V2 l = project(left);
            V2 r = project(right);

            line_bresenham_algo(
                l.x, l.y,
                r.x, r.y
            );
        }
    }
}

void draw_horizon(void)
{
    line_bresenham_algo(
        0,
        35,
        127,
        35
    );
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
