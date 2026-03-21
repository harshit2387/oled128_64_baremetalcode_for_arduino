#ifndef OLED_H
#define OLED_H

#include "i2c.h"

#define OLED_ADDR 0x3C   // 7-bit address

uint8_t buffer[1024];

/* ================== COMMAND ================== */
void oled_command(uint8_t cmd)
{
    i2c_start();
    i2c_write(OLED_ADDR << 1);
    i2c_write(0x00);
    i2c_write(cmd);
    i2c_stop();
}

/* ================== DATA ================== */
void oled_data(uint8_t data)
{
    i2c_start();
    i2c_write(OLED_ADDR << 1);
    i2c_write(0x40);
    i2c_write(data);
    i2c_stop();
}

/* ================== INIT ================== */
void oled_init()
{
    oled_command(0xAE);
    oled_command(0x20);
    oled_command(0x10);
    oled_command(0xB0);
    oled_command(0xC8);
    oled_command(0x00);
    oled_command(0x10);
    oled_command(0x40);
    oled_command(0x81);
    oled_command(0xFF);
    oled_command(0xA1);
    oled_command(0xA6);
    oled_command(0xA8);
    oled_command(0x3F);
    oled_command(0xA4);
    oled_command(0xD3);
    oled_command(0x00);
    oled_command(0xD5);
    oled_command(0xF0);
    oled_command(0xD9);
    oled_command(0x22);
    oled_command(0xDA);
    oled_command(0x12);
    oled_command(0xDB);
    oled_command(0x20);
    oled_command(0x8D);
    oled_command(0x14);
    oled_command(0xAF);
}

/* ================== CLEAR ================== */
void oled_clear()
{
    for (int i = 0; i < 1024; i++)
        buffer[i] = 0x00;
}

/* ================== FAST UPDATE (BURST MODE) ================== */
void oled_update()
{
    for (uint8_t page = 0; page < 8; page++)
    {
        oled_command(0xB0 + page);
        oled_command(0x00);
        oled_command(0x10);

        i2c_start();
        i2c_write(OLED_ADDR << 1);
        i2c_write(0x40);

        for (uint8_t col = 0; col < 128; col++)
        {
            i2c_write(buffer[page * 128 + col]);
        }

        i2c_stop();
    }
}

/* ================== PIXEL ================== */
void oled_pixel(uint8_t x, uint8_t y)
{
    if (x >= 128 || y >= 64) return;

    uint16_t index = x + (y / 8) * 128;
    buffer[index] |= (1 << (y % 8));
}

#endif