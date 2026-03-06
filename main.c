#include "OLED.h"

int main()
{
    oled_init();

    oled_clear();

    oled_pixel(20,20);
    oled_pixel(30,20);
    oled_pixel(40,20);

    oled_update();

    while(1);
}