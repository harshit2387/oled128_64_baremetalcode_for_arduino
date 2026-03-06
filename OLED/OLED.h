#ifndef OLED_H
#define OLED_H

#include <stdint.h>

#define OLED_ADDR 0x78

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES 8

#define OLED_BUFFER_SIZE (OLED_WIDTH * OLED_PAGES)

void oled_init(void);
void oled_clear(void);
void oled_update(void);

void oled_pixel(uint8_t x, uint8_t y);
void oled_pixel_clear(uint8_t x, uint8_t y);
void oled_pixel_toggle(uint8_t x, uint8_t y);

#endif