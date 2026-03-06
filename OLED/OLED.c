#include "OLED.h"
#include "I2C.h"

static uint8_t oled_buffer[OLED_BUFFER_SIZE];

static void oled_command(uint8_t cmd)
{
    i2c_start();

    i2c_send_data(OLED_ADDR);
    i2c_send_data(0x00);
    i2c_send_data(cmd);

    i2c_stop();
}

static void oled_data(uint8_t data)
{
    i2c_start();

    i2c_send_data(OLED_ADDR);
    i2c_send_data(0x40);
    i2c_send_data(data);

    i2c_stop();
}

void oled_init()
{
    i2c_init();

    oled_command(0xAE);
    oled_command(0xD5);
    oled_command(0x80);
    oled_command(0xA8);
    oled_command(0x3F);
    oled_command(0xD3);
    oled_command(0x00);
    oled_command(0x40);
    oled_command(0x8D);
    oled_command(0x14);
    oled_command(0x20);
    oled_command(0x00);
    oled_command(0xA1);
    oled_command(0xC8);
    oled_command(0xDA);
    oled_command(0x12);
    oled_command(0x81);
    oled_command(0xCF);
    oled_command(0xD9);
    oled_command(0xF1);
    oled_command(0xDB);
    oled_command(0x40);
    oled_command(0xA4);
    oled_command(0xA6);
    oled_command(0xAF);
}

void oled_clear()
{
    for (uint16_t i = 0; i < OLED_BUFFER_SIZE; i++)
        oled_buffer[i] = 0x00;
}

void oled_pixel(uint8_t x, uint8_t y)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;

    uint16_t index = x + (y / 8) * OLED_WIDTH;
    oled_buffer[index] |= (1 << (y % 8));
}

void oled_pixel_clear(uint8_t x, uint8_t y)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;

    uint16_t index = x + (y / 8) * OLED_WIDTH;
    oled_buffer[index] &= ~(1 << (y % 8));
}

void oled_pixel_toggle(uint8_t x, uint8_t y)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;

    uint16_t index = x + (y / 8) * OLED_WIDTH;
    oled_buffer[index] ^= (1 << (y % 8));
}

void oled_update()
{
    for (uint8_t page = 0; page < OLED_PAGES; page++)
    {
        oled_command(0xB0 + page);
        oled_command(0x00);
        oled_command(0x10);

        i2c_start();

        i2c_send_data(OLED_ADDR);
        i2c_send_data(0x40);

        for (uint8_t col = 0; col < OLED_WIDTH; col++)
        {
            i2c_send_data(oled_buffer[page * OLED_WIDTH + col]);
        }

        i2c_stop();
    }
}