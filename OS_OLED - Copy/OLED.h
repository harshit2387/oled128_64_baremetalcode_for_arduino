#ifndef OLED_H
#define OLED_H

#include <stdint.h>
#include "i2c.h"

#define OLED_ADDR 0x3C

/* ================== BUFFER ================== */
static uint8_t buffer[1024];

/* ================== CURSOR ================== */
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

/* ================== FONT ================== */
static const uint8_t font5x7[128][5] = {
    ['0'] = {0x3E,0x51,0x49,0x45,0x3E},
    ['1'] = {0x00,0x42,0x7F,0x40,0x00},
    ['2'] = {0x42,0x61,0x51,0x49,0x46},
    ['3'] = {0x21,0x41,0x45,0x4B,0x31},
    ['4'] = {0x18,0x14,0x12,0x7F,0x10},
    ['5'] = {0x27,0x45,0x45,0x45,0x39},
    ['6'] = {0x3C,0x4A,0x49,0x49,0x30},
    ['7'] = {0x01,0x71,0x09,0x05,0x03},
    ['8'] = {0x36,0x49,0x49,0x49,0x36},
    ['9'] = {0x06,0x49,0x49,0x29,0x1E},

    ['A'] = {0x7E,0x11,0x11,0x11,0x7E},
    ['B'] = {0x7F,0x49,0x49,0x49,0x36},
    ['C'] = {0x3E,0x41,0x41,0x41,0x22},
    ['E'] = {0x7F,0x49,0x49,0x49,0x41},
    ['F'] = {0x7F,0x09,0x09,0x09,0x01},
    ['L'] = {0x7F,0x40,0x40,0x40,0x40},
    ['M'] = {0x7F,0x02,0x04,0x02,0x7F},
    ['N'] = {0x7F,0x04,0x08,0x10,0x7F},
    ['O'] = {0x3E,0x41,0x41,0x41,0x3E},
    ['S'] = {0x46,0x49,0x49,0x49,0x31},
    ['U'] = {0x3F,0x40,0x40,0x40,0x3F},
    ['Y'] = {0x07,0x08,0x70,0x08,0x07},

    ['a'] = {0x20,0x54,0x54,0x54,0x78},
    ['e'] = {0x38,0x54,0x54,0x54,0x18},
    ['k'] = {0x7F,0x10,0x28,0x44,0x00},

    [':'] = {0x00,0x36,0x36,0x00,0x00},
};

/* ================== LOW LEVEL ================== */
static void oled_command(uint8_t cmd)
{
    i2c_start();
    i2c_write(OLED_ADDR << 1);
    i2c_write(0x00);
    i2c_write(cmd);
    i2c_stop();
}

static void oled_data(uint8_t data)
{
    i2c_start();
    i2c_write(OLED_ADDR << 1);
    i2c_write(0x40);
    i2c_write(data);
    i2c_stop();
}

/* ================== INIT ================== */
static void oled_init()
{
    oled_command(0xAE);
    oled_command(0x20); oled_command(0x10);
    oled_command(0xB0);
    oled_command(0xC8);
    oled_command(0x00);
    oled_command(0x10);
    oled_command(0x40);
    oled_command(0x81); oled_command(0xFF);
    oled_command(0xA1);
    oled_command(0xA6);
    oled_command(0xA8); oled_command(0x3F);
    oled_command(0xA4);
    oled_command(0xD3); oled_command(0x00);
    oled_command(0xD5); oled_command(0xF0);
    oled_command(0xD9); oled_command(0x22);
    oled_command(0xDA); oled_command(0x12);
    oled_command(0xDB); oled_command(0x20);
    oled_command(0x8D); oled_command(0x14);
    oled_command(0xAF);
}

/* ================== CLEAR ================== */
static void oled_clear()
{
    for (uint16_t i = 0; i < 1024; i++)
        buffer[i] = 0x00;
}

/* ================== UPDATE ================== */
static void oled_update()
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
            i2c_write(buffer[page * 128 + col]);

        i2c_stop();
    }
}

/* ================== PIXEL ================== */
static void oled_pixel(uint8_t x, uint8_t y)
{
    if (x >= 128 || y >= 64) return;

    uint16_t index = x + (y / 8) * 128;
    buffer[index] |= (1 << (y % 8));
}

/* ================== CURSOR ================== */
static void oled_set_cursor(uint8_t x, uint8_t y)
{
    cursor_x = x;
    cursor_y = y;
}

/* ================== CHAR ================== */
static void oled_char(char c)
{
    if (c < 0 || c > 127) return;

    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t line = font5x7[(uint8_t)c][i];

        for (uint8_t j = 0; j < 8; j++)
        {
            if (line & (1 << j))
                oled_pixel(cursor_x + i, cursor_y * 8 + j);
        }
    }

    cursor_x += 6;
}

/* ================== STRING ================== */
static void oled_print(const char* str)
{
    while (*str)
        oled_char(*str++);
}

/* ================== NUMBER ================== */
static void oled_print_num(uint16_t num)
{
    char buf[6];
    uint8_t i = 0;

    if (num == 0)
    {
        oled_char('0');
        return;
    }

    while (num > 0)
    {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }

    while (i--)
        oled_char(buf[i]);
}

#endif