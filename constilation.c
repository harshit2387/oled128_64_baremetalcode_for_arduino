#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "I2C.h"
#include "OLED.h"

#define SCALE 1024L

typedef struct
{
    long x;
    long y;
} Vec2;


/* Draw star */

void star(Vec2 p)
{
    oled_pixel(p.x,p.y);

    oled_pixel(p.x-1,p.y);
    oled_pixel(p.x+1,p.y);

    oled_pixel(p.x,p.y-1);
    oled_pixel(p.x,p.y+1);

    oled_pixel(p.x-1,p.y-1);
    oled_pixel(p.x+1,p.y+1);

    oled_pixel(p.x-1,p.y+1);
    oled_pixel(p.x+1,p.y-1);
}



/* Degree-2 quadratic curve */

void poly2(Vec2 p0,Vec2 p1,Vec2 p2)
{
    for(long t=0;t<=SCALE;t+=2)
    {
        long u=SCALE-t;


        long x=
        (
        u*u*p0.x
        +
        2*u*t*p1.x
        +
        t*t*p2.x
        )
        /(SCALE*SCALE);



        long y=
        (
        u*u*p0.y
        +
        2*u*t*p1.y
        +
        t*t*p2.y
        )
        /(SCALE*SCALE);



        if(x>=0 && x<128 &&
           y>=0 && y<64)
        {
            oled_pixel(
                (uint8_t)x,
                (uint8_t)y
            );
        }
    }
}



int main()
{
    i2c_init();

    oled_init();


    Vec2 stars[7]=
    {
        {8,10},      // Polaris
        {35,7},      // Yildun
        {52,18},     // Urodelus
        {84,34},     // Akhfa
        {92,48},     // Anwar
        {120,30},    // Kochab
        {112,52}     // Pherkad
    };


    while(1)
    {
        oled_clear();


        /* Draw all stars */

        for(uint8_t i=0;i<7;i++)
        {
            star(stars[i]);
        }


        /*
           Single quadratic curve:

           Polaris ----> Urodelus ----> Pherkad

           p0=start
           p1=control
           p2=end
        */

        poly2(
            stars[0],   // Polaris
            stars[6],   // Urodelus
            stars[1]    // Pherkad
        );


        oled_update();

        _delay_ms(100);
    }
}











/*
Polaris --- Yildun
                \
                 \
              Urodelus
                     \
                      Akhfa -------- Kochab
                        \          /
                         \        /
                       Anwar --- Pherkad  */
