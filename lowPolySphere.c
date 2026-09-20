#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "I2C.h"
#include "OLED.h"

#define WIDTH       128
#define HEIGHT       64

#define CX          64
#define CY          32

#define SCALE       45.0f
#define CAMERA_Z    65.0f

#define PI          3.14159265f


/* =========================================================
   3D VECTOR
   ========================================================= */

typedef struct
{
    float x;
    float y;
    float z;

} Vec3;


/* =========================================================
   2D POINT
   ========================================================= */

typedef struct
{
    int16_t x;
    int16_t y;

} Vec2;


/* =========================================================
   QUATERNION
   ========================================================= */

typedef struct
{
    float w;
    float x;
    float y;
    float z;

} Quaternion;


/* =========================================================
   LOW POLY SPHERE
   =========================================================

                 0
                /|\
               / | \
              /  |  \
             1---2---3
              \  |  /
               \ | /
                \|/
                 4

   plus middle/back vertices.

   We use 20 vertices arranged as:

       top       = 0

       upper ring = 1..6

       lower ring = 7..12

       bottom    = 13

       center/back structure = 14..19

   ========================================================= */


/*
   20 vertices
*/

#define VERTEX_COUNT 20

Vec3 vertices[VERTEX_COUNT] =
{
    /* Top */

    {  0, -20,   0 },


    /* Upper ring */

    { 17, -10,   0 },
    {  8, -10,  15 },
    { -8, -10,  15 },
    {-17, -10,   0 },
    { -8, -10, -15 },
    {  8, -10, -15 },


    /* Lower ring */

    { 17,  10,   0 },
    {  8,  10,  15 },
    { -8,  10,  15 },
    {-17,  10,   0 },
    { -8,  10, -15 },
    {  8,  10, -15 },


    /* Bottom */

    {  0,  20,   0 },


    /* Extra middle vertices */

    {  0,   0,  20 },
    { 20,   0,   0 },
    {  0,   0, -20 },
    {-20,   0,   0 },

    {  0,   0,  10 },
    {  0,   0, -10 }
};


/* =========================================================
   TRIANGLE LIST
   =========================================================

   Every face consists of 3 vertices.

   We only need the edges of these triangles because
   this is a wireframe renderer.
   ========================================================= */

#define TRIANGLE_COUNT 24

uint8_t triangles[TRIANGLE_COUNT][3] =
{
    /* Top */

    {0,1,2},
    {0,2,3},
    {0,3,4},
    {0,4,5},
    {0,5,6},
    {0,6,1},


    /* Upper / middle */

    {1,7,8},
    {1,8,2},

    {2,8,9},
    {2,9,3},

    {3,9,10},
    {3,10,4},

    {4,10,11},
    {4,11,5},

    {5,11,12},
    {5,12,6},

    {6,12,7},
    {6,7,1},


    /* Bottom */

    {13,8,7},
    {13,9,8},
    {13,10,9},
    {13,11,10},
    {13,12,11},
    {13,7,12}
};


/* =========================================================
   ROTATED VERTICES
   ========================================================= */

Vec3 rotated[VERTEX_COUNT];

Vec2 projected[VERTEX_COUNT];

uint8_t visible[VERTEX_COUNT];


/* =========================================================
   QUATERNION MULTIPLICATION
   ========================================================= */

Quaternion quatMultiply(
    Quaternion a,
    Quaternion b
)
{
    Quaternion q;

    q.w =
        a.w*b.w -
        a.x*b.x -
        a.y*b.y -
        a.z*b.z;

    q.x =
        a.w*b.x +
        a.x*b.w +
        a.y*b.z -
        a.z*b.y;

    q.y =
        a.w*b.y -
        a.x*b.z +
        a.y*b.w +
        a.z*b.x;

    q.z =
        a.w*b.z +
        a.x*b.y -
        a.y*b.x +
        a.z*b.w;

    return q;
}


/* =========================================================
   QUATERNION NORMALIZATION
   ========================================================= */

Quaternion quatNormalize(
    Quaternion q
)
{
    float len;

    len =
        sqrtf(
            q.w*q.w +
            q.x*q.x +
            q.y*q.y +
            q.z*q.z
        );

    if (len > 0.0001f)
    {
        q.w /= len;
        q.x /= len;
        q.y /= len;
        q.z /= len;
    }

    return q;
}


/* =========================================================
   AXIS ANGLE
   ========================================================= */

Quaternion quatAxisAngle(
    float x,
    float y,
    float z,
    float angle
)
{
    Quaternion q;

    float half = angle * 0.5f;

    float s = sinf(half);

    q.w = cosf(half);

    q.x = x * s;
    q.y = y * s;
    q.z = z * s;

    return q;
}


/* =========================================================
   QUATERNION CONJUGATE
   ========================================================= */

Quaternion quatConjugate(
    Quaternion q
)
{
    Quaternion r;

    r.w = q.w;

    r.x = -q.x;
    r.y = -q.y;
    r.z = -q.z;

    return r;
}


/* =========================================================
   ROTATE VECTOR
   ========================================================= */

Vec3 quatRotate(
    Quaternion q,
    Vec3 v
)
{
    Quaternion p;

    p.w = 0;

    p.x = v.x;
    p.y = v.y;
    p.z = v.z;

    Quaternion qc =
        quatConjugate(q);

    Quaternion a =
        quatMultiply(q, p);

    Quaternion b =
        quatMultiply(a, qc);

    Vec3 result;

    result.x = b.x;
    result.y = b.y;
    result.z = b.z;

    return result;
}


/* =========================================================
   PROJECT 3D -> OLED
   ========================================================= */

uint8_t project(
    Vec3 p,
    Vec2 *out
)
{
    float z;

    z =
        CAMERA_Z + p.z;


    /*
       Don't render points behind camera
    */

    if (z <= 5.0f)
        return 0;


    out->x =
        CX +
        (int16_t)(
            p.x * SCALE / z
        );


    out->y =
        CY +
        (int16_t)(
            p.y * SCALE / z
        );


    /*
       Reject points far outside screen
    */

    if (
        out->x < -5 ||
        out->x > WIDTH + 5 ||
        out->y < -5 ||
        out->y > HEIGHT + 5
    )
    {
        return 0;
    }

    return 1;
}


/* =========================================================
   BRESENHAM LINE
   ========================================================= */

void drawLine(
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1
)
{
    int16_t dx =
        abs(x1 - x0);

    int16_t sx =
        (x0 < x1) ? 1 : -1;

    int16_t dy =
        -abs(y1 - y0);

    int16_t sy =
        (y0 < y1) ? 1 : -1;

    int16_t err =
        dx + dy;


    while (1)
    {
        if (
            x0 >= 0 &&
            x0 < WIDTH &&
            y0 >= 0 &&
            y0 < HEIGHT
        )
        {
            oled_pixel(
                (uint8_t)x0,
                (uint8_t)y0
            );
        }


        if (
            x0 == x1 &&
            y0 == y1
        )
        {
            break;
        }


        int16_t e2 =
            2 * err;


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


/* =========================================================
   DRAW TRIANGLE
   ========================================================= */

void drawTriangle(
    uint8_t a,
    uint8_t b,
    uint8_t c
)
{
    if (
        !visible[a] ||
        !visible[b] ||
        !visible[c]
    )
    {
        return;
    }


    /*
       Edge AB
    */

    drawLine(
        projected[a].x,
        projected[a].y,

        projected[b].x,
        projected[b].y
    );


    /*
       Edge BC
    */

    drawLine(
        projected[b].x,
        projected[b].y,

        projected[c].x,
        projected[c].y
    );


    /*
       Edge CA
    */

    drawLine(
        projected[c].x,
        projected[c].y,

        projected[a].x,
        projected[a].y
    );
}


/* =========================================================
   DRAW OBJECT
   ========================================================= */

void drawObject(void)
{
    for (uint8_t i = 0;
         i < TRIANGLE_COUNT;
         i++)
    {
        drawTriangle(
            triangles[i][0],
            triangles[i][1],
            triangles[i][2]
        );
    }
}


/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    /*
       I2C
    */

    i2c_init();


    /*
       OLED
    */

    oled_init();


    /*
       Initial quaternion
    */

    Quaternion rotation;

    rotation.w = 1.0f;

    rotation.x = 0.0f;
    rotation.y = 0.0f;
    rotation.z = 0.0f;


    while (1)
    {
        /*
           Clear OLED
        */

        oled_clear();


        /*
           ---------------------------------------------
           ROTATE + PROJECT ALL VERTICES
           ---------------------------------------------
        */

        for (uint8_t i = 0;
             i < VERTEX_COUNT;
             i++)
        {
            rotated[i] =
                quatRotate(
                    rotation,
                    vertices[i]
                );


            visible[i] =
                project(
                    rotated[i],
                    &projected[i]
                );
        }


        /*
           ---------------------------------------------
           DRAW TRIANGLES
           ---------------------------------------------
        */

        drawObject();


        /*
           ---------------------------------------------
           UPDATE OLED
           ---------------------------------------------
        */

        oled_update();


        /*
           ---------------------------------------------
           ROTATE
           ---------------------------------------------
        */

        Quaternion qx =
            quatAxisAngle(
                1.0f,
                0.0f,
                0.0f,
                0.035f
            );


        Quaternion qy =
            quatAxisAngle(
                0.0f,
                1.0f,
                0.0f,
                0.045f
            );


        rotation =
            quatMultiply(
                qx,
                rotation
            );


        rotation =
            quatMultiply(
                qy,
                rotation
            );


        /*
           Normalize
        */

        rotation =
            quatNormalize(rotation);


        _delay_ms(50);
    }

    return 0;
}
