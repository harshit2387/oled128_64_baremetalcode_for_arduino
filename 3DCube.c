#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <math.h>

#include "I2C.h"
#include "OLED.h"

#define SCALE 64

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} Vec3;

typedef struct
{
    int16_t x;
    int16_t y;
} Vec2;


/* Cube vertices */
Vec3 cube[8]=
{
    {-10,-10, 10},
    { 10,-10, 10},
    { 10, 10, 10},
    {-10, 10, 10},

    {-10,-10,-10},
    { 10,-10,-10},
    { 10, 10,-10},
    {-10, 10,-10}
};


/* edge table */
uint8_t edges[12][2]=
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



void line(
    int x0,
    int y0,
    int x1,
    int y1
)
{
    int dx=abs(x1-x0);
    int sx=x0<x1?1:-1;

    int dy=-abs(y1-y0);
    int sy=y0<y1?1:-1;

    int err=dx+dy;

    while(1)
    {
        oled_pixel(x0,y0);

        if(x0==x1 && y0==y1)
            break;

        int e2=2*err;

        if(e2>=dy)
        {
            err+=dy;
            x0+=sx;
        }

        if(e2<=dx)
        {
            err+=dx;
            y0+=sy;
        }
    }
}



void rotateProject(
    Vec3 p,
    Vec2 *out,
    float ax,
    float ay
)
{
    float x=p.x;
    float y=p.y;
    float z=p.z;


    /* rotate Y */
    float x1=
        x*cos(ay)+
        z*sin(ay);

    float z1=
        z*cos(ay)-
        x*sin(ay);


    /* rotate X */

    float y1=
        y*cos(ax)-
        z1*sin(ax);

    float z2=
        z1*cos(ax)+
        y*sin(ax);



    /* perspective */

    int d=50;

    out->x=
        (x1*SCALE)/(z2+d)+64;

    out->y=
        (y1*SCALE)/(z2+d)+32;
}




int main()
{
    i2c_init();

    oled_init();

    float ax=0;
    float ay=0;

    Vec2 p[8];

    while(1)
    {
        oled_clear();


        for(uint8_t i=0;i<8;i++)
        {
            rotateProject(
                cube[i],
                &p[i],
                ax,
                ay
            );
        }


        for(uint8_t i=0;i<12;i++)
        {
            uint8_t a=
            edges[i][0];

            uint8_t b=
            edges[i][1];


            line(
                p[a].x,
                p[a].y,
                p[b].x,
                p[b].y
            );
        }


        oled_update();


        ax+=0.05;

        ay+=0.08;


        if(ax>6.28)
            ax=0;

        if(ay>6.28)
            ay=0;


        _delay_ms(30);
    }
}
