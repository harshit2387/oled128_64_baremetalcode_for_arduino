#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <math.h>

#include "I2C.h"
#include "OLED.h"

#define OLED_WIDTH  128
#define OLED_HEIGHT 64

#define CENTER_X 64
#define CENTER_Y 32

#define SCALE 45
#define CAMERA_Z 60

typedef struct
{
    float x;
    float y;
    float z;
} Vec3;

typedef struct
{
    int16_t x;
    int16_t y;
} Vec2;


/* --------------------------------------------------
   Cube vertices
   -------------------------------------------------- */

Vec3 cube[8] =
{
    {-20, -20,  20},   // 0
    { 20, -20,  20},   // 1
    { 20,  20,  20},   // 2
    {-20,  20,  20},   // 3

    {-20, -20, -20},   // 4
    { 20, -20, -20},   // 5
    { 20,  20, -20},   // 6
    {-20,  20, -20}    // 7
};


/* --------------------------------------------------
   Cube edges
   -------------------------------------------------- */

uint8_t edges[12][2] =
{
    {0,1},
    {1,2},
    {2,3},
    {3,0},

    {4,5},
    {5,6},
    {6,7},
    {7,4},

    {0,4},
    {1,5},
    {2,6},
    {3,7}
};


/* --------------------------------------------------
   Diagonal lines on cube faces
   -------------------------------------------------- */

uint8_t diagonals[6][2] =
{
    {0,2},     // front
    {4,6},     // back

    {0,5},     // bottom
    {3,6},     // top

    {0,7},     // left
    {1,6}      // right
};


/* --------------------------------------------------
   Bresenham line
   -------------------------------------------------- */

void drawLine(int16_t x0, int16_t y0,
              int16_t x1, int16_t y1)
{
    int16_t dx = abs(x1 - x0);
    int16_t sx = (x0 < x1) ? 1 : -1;

    int16_t dy = -abs(y1 - y0);
    int16_t sy = (y0 < y1) ? 1 : -1;

    int16_t err = dx + dy;

    while (1)
    {
        if (x0 >= 0 && x0 < OLED_WIDTH &&
            y0 >= 0 && y0 < OLED_HEIGHT)
        {
            oled_pixel((uint8_t)x0, (uint8_t)y0);
        }

        if (x0 == x1 && y0 == y1)
            break;

        int16_t e2 = 2 * err;

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


/* --------------------------------------------------
   Rotate around X axis
   -------------------------------------------------- */

Vec3 rotateX(Vec3 p, float angle)
{
    Vec3 r;

    float c = cosf(angle);
    float s = sinf(angle);

    r.x = p.x;

    r.y = p.y * c - p.z * s;
    r.z = p.y * s + p.z * c;

    return r;
}


/* --------------------------------------------------
   Rotate around Y axis
   -------------------------------------------------- */

Vec3 rotateY(Vec3 p, float angle)
{
    Vec3 r;

    float c = cosf(angle);
    float s = sinf(angle);

    r.x = p.x * c + p.z * s;

    r.y = p.y;

    r.z = -p.x * s + p.z * c;

    return r;
}


/* --------------------------------------------------
   Rotate around Z axis
   -------------------------------------------------- */

Vec3 rotateZ(Vec3 p, float angle)
{
    Vec3 r;

    float c = cosf(angle);
    float s = sinf(angle);

    r.x = p.x * c - p.y * s;
    r.y = p.x * s + p.y * c;
    r.z = p.z;

    return r;
}


/* --------------------------------------------------
   3D -> 2D perspective projection
   -------------------------------------------------- */

Vec2 project(Vec3 p)
{
    Vec2 result;

    float z = p.z + CAMERA_Z;

    if (z < 1)
        z = 1;

    result.x =
        CENTER_X +
        (int16_t)((p.x * SCALE) / z);

    result.y =
        CENTER_Y +
        (int16_t)((p.y * SCALE) / z);

    return result;
}


/* --------------------------------------------------
   Main
   -------------------------------------------------- */

int main(void)
{
    i2c_init();
    oled_init();

    float angleX = 0.0f;
    float angleY = 0.0f;
    float angleZ = 0.0f;

    Vec3 rotated[8];
    Vec2 projected[8];

    while (1)
    {
        oled_clear();


        /* ------------------------------------------
           Rotate all cube vertices
           ------------------------------------------ */

        for (uint8_t i = 0; i < 8; i++)
        {
            Vec3 p = cube[i];

            p = rotateX(p, angleX);
            p = rotateY(p, angleY);
            p = rotateZ(p, angleZ);

            rotated[i] = p;

            projected[i] = project(p);
        }


        /* ------------------------------------------
           Draw normal cube edges
           ------------------------------------------ */

        for (uint8_t i = 0; i < 12; i++)
        {
            uint8_t a = edges[i][0];
            uint8_t b = edges[i][1];

            drawLine(
                projected[a].x,
                projected[a].y,
                projected[b].x,
                projected[b].y
            );
        }


        /* ------------------------------------------
           Draw diagonal face lines
           ------------------------------------------ */

        for (uint8_t i = 0; i < 6; i++)
        {
            uint8_t a = diagonals[i][0];
            uint8_t b = diagonals[i][1];

            drawLine(
                projected[a].x,
                projected[a].y,
                projected[b].x,
                projected[b].y
            );
        }


        /* ------------------------------------------
           Send framebuffer to OLED
           ------------------------------------------ */

        oled_update();


        /* ------------------------------------------
           Rotation
           ------------------------------------------ */

        angleX += 0.04f;
        angleY += 0.06f;
        angleZ += 0.02f;

        if (angleX > 6.28f)
            angleX = 0;

        if (angleY > 6.28f)
            angleY = 0;

        if (angleZ > 6.28f)
            angleZ = 0;


        _delay_ms(40);
    }

    return 0;
}
