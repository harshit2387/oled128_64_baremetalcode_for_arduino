// main.c
#include "oled.h"
#include "i2c.h"
#include <util/delay.h>

// Example bitmaps (8x8 pixels each)
static const uint8_t eye_open[8] = {
    0b00111100,
    0b01000010,
    0b10100101,
    0b10000001,
    0b10100101,
    0b10011001,
    0b01000010,
    0b00111100
};

static const uint8_t eye_closed[8] = {
    0b00000000,
    0b00000000,
    0b11111111,
    0b00000000,
    0b00000000,
    0b11111111,
    0b00000000,
    0b00000000
};

static const uint8_t mouth_smile[8] = {
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b10000001,
    0b01000010,
    0b00111100,
    0b00000000
};

// --- Helper: Draw Bitmap ---
void draw_bitmap(oled* display, uint8_t x, uint8_t y,
                 const uint8_t* bitmap, uint8_t w, uint8_t h) {
    for (uint8_t row = 0; row < h; row++) {
        for (uint8_t col = 0; col < w; col++) {
            bool pixel_on = (bitmap[row] >> (7 - col)) & 0x01;
            if (pixel_on) {
                add_pixel(display, x + col, y + row, true);
            }
        }
    }
}

int main(void) {
    oled display;
    oled_init(&display);

    while (1) {
        oled_clear(&display);

        // Draw open eyes + smile
        draw_bitmap(&display, 20, 20, eye_open, 8, 8);
        draw_bitmap(&display, 100, 20, eye_open, 8, 8);
        draw_bitmap(&display, 50, 40, mouth_smile, 8, 8);

        oled_render(&display);
        oled_update(&display);
        _delay_ms(1000);

        oled_clear(&display);

        // Draw closed eyes + smile
        draw_bitmap(&display, 20, 20, eye_closed, 8, 8);
        draw_bitmap(&display, 100, 20, eye_closed, 8, 8);
        draw_bitmap(&display, 50, 40, mouth_smile, 8, 8);

        oled_render(&display);
        oled_update(&display);
        _delay_ms(1000);
    }
}