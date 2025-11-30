#ifndef OLED_H
#define OLED_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include "i2c.h"
// --- Pixel Node for Linked List ---
typedef struct PixelNode {
    uint8_t x;
    uint8_t y;
    bool color;
    struct PixelNode* next;
} PixelNode;

// --- OLED Display Struct ---
typedef struct oled { 
    uint8_t row;
    uint8_t column;
    uint8_t address;
    uint8_t width;
    uint8_t height;
    PixelNode* head;   // linked list of pixels
    struct oled* next; // optional chaining of multiple displays
} oled;

// --- Framebuffer ---
static uint8_t oled_buffer[128 * 64 / 8]; // array is used because of dwaring can be easy 

// --- I2C Command Helper ---
static inline void oled_command(oled *display, uint8_t command) {
    I2C_start();
    I2C_write((display->address << 1) | 0); // Write mode
    I2C_write(0x00); // Control byte: command
    I2C_write(command);
    I2C_stop();
}

// --- OLED Initialization ---
static inline void oled_init(oled* display) {
    display->address = 0x3C;
    display->width   = 128;
    display->height  = 64;
    display->head    = NULL;
    display->next    = NULL;

    I2C_init();

    oled_command(display, 0xAE); // Display OFF
    oled_command(display, 0x20); oled_command(display, 0x00); // Horizontal addressing mode
    oled_command(display, 0xB0);
    oled_command(display, 0xC8);
    oled_command(display, 0x00);
    oled_command(display, 0x10);
    oled_command(display, 0x40);
    oled_command(display, 0x81); oled_command(display, 0x7F);
    oled_command(display, 0xA1);
    oled_command(display, 0xA6);
    oled_command(display, 0xA8); oled_command(display, 0x3F);
    oled_command(display, 0xA4);
    oled_command(display, 0xD3); oled_command(display, 0x00);
    oled_command(display, 0xD5); oled_command(display, 0x80);
    oled_command(display, 0xD9); oled_command(display, 0xF1);
    oled_command(display, 0xDA); oled_command(display, 0x12);
    oled_command(display, 0xDB); oled_command(display, 0x40);
    oled_command(display, 0x8D); oled_command(display, 0x14);
    oled_command(display, 0xAF); // Display ON

    memset(oled_buffer, 0x00, sizeof(oled_buffer));
}

// --- Clear Linked List + Buffer ---
static inline void oled_clear(oled* display) {
    PixelNode* current = display->head;
    while (current != NULL) {
        PixelNode* tmp = current;
        current = current->next;
        free(tmp);
    }
    display->head = NULL;
    memset(oled_buffer, 0x00, sizeof(oled_buffer));
}

// --- Add Pixel Node ---
static inline void add_pixel(oled* display, uint8_t x, uint8_t y, bool color) {
    if (x >= display->width || y >= display->height) return;
    PixelNode* newNode = (PixelNode*)malloc(sizeof(PixelNode)); //Heap allocation
    if (!newNode) return; // allocation failed //SRAM 2KB
    newNode->x = x;
    newNode->y = y;
    newNode->color = color;
    newNode->next = display->head;
    display->head = newNode;
}

// --- Remove Pixel Node ---
static inline void remove_pixel(oled* display, uint8_t x, uint8_t y) {
    PixelNode* current = display->head;
    PixelNode* prev = NULL; //Heap deallocation
    while (current != NULL) {
        if (current->x == x && current->y == y) {
            if (prev == NULL) {
                display->head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

// --- Render Linked List into Framebuffer ---
static inline void oled_render(oled* display) {
    memset(oled_buffer, 0x00, sizeof(oled_buffer));

    PixelNode* current = display->head;
    while (current != NULL) {
        uint16_t byte_index = current->x + (current->y / 8) * display->width;
        uint8_t bit_index = current->y % 8;

        if (current->color) {
            oled_buffer[byte_index] |= (1 << bit_index);
        } else {
            oled_buffer[byte_index] &= ~(1 << bit_index);
        }
        current = current->next;
    }
}

// --- Push Framebuffer to OLED via I2C ---
static inline void oled_update(oled* display) {
    for (uint8_t page = 0; page < (display->height / 8); page++) {
        oled_command(display, 0xB0 + page); // Set page address
        oled_command(display, 0x00);        // Set lower column
        oled_command(display, 0x10);        // Set higher column

        I2C_start();
        I2C_write((display->address << 1) | 0); // Write mode
        I2C_write(0x40); // Control byte: data

        for (uint8_t col = 0; col < display->width; col++) {
            I2C_write(oled_buffer[page * display->width + col]);
        }

        I2C_stop();
    }
}

#endif // OLED_H