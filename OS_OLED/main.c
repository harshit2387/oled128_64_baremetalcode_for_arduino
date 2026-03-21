#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#include "I2C.h"
#include "OLED.h"
#include "Malloc.h"

/* ================== NODE ================== */
typedef struct OLED_Node
{
    uint8_t x;
    uint8_t y;
    struct OLED_Node* next;
} OLED_Node;

OLED_Node* head = 0;

/* ================== CREATE NODE ================== */
OLED_Node* create_node(uint8_t x, uint8_t y)
{
    OLED_Node* node = (OLED_Node*)my_malloc(sizeof(OLED_Node));

    if (!node)
    {
        // DEBUG: show error pixel if malloc fails
        oled_clear();
        oled_pixel(0, 0);
        oled_update();
        while (1);
    }

    node->x = x;
    node->y = y;
    node->next = 0;

    return node;
}

/* ================== ADD PIXEL ================== */
void add_pixel(uint8_t x, uint8_t y)
{
    OLED_Node* new_node = create_node(x, y);

    if (!head)
    {
        head = new_node;
    }
    else
    {
        OLED_Node* temp = head;
        while (temp->next)
            temp = temp->next;

        temp->next = new_node;
    }
}

/* ================== RENDER ================== */
void render_pixels()
{
    OLED_Node* temp = head;

    while (temp)
    {
        oled_pixel(temp->x, temp->y);
        temp = temp->next;
    }
}

/* ================== MAIN ================== */
int main(void)
{
    i2c_init();
    oled_init();

    /* Add pixels using custom malloc */
    add_pixel(64, 32);   // center pixel
    add_pixel(10, 10);
    add_pixel(20, 20);
    add_pixel(30, 30);

    while (1)
    {
        oled_clear();      // clear buffer
        render_pixels();   // draw from linked list
        oled_update();     // send to OLED

        _delay_ms(500);
    }
}