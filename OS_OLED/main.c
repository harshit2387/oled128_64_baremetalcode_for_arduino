#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#include "I2C.h"
#include "OLED.h"
#include "Malloc.h"

/* ===== LINKER SYMBOLS ===== */
extern char __bss_end;
extern char *__brkval;

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
        head = new_node;
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

/* ================== HEAP VISUAL ================== */
void draw_heap_map(void)
{
    oled_clear();

    uint16_t heap_start = (uint16_t)&__bss_end;
    uint16_t heap_end;

    if (__brkval == 0)
        heap_end = heap_start;
    else
        heap_end = (uint16_t)__brkval;

    uint16_t addr = heap_start;
    uint8_t x = 0;
    uint8_t y = 0;

    while (addr < heap_end)
    {
        block_t* block = (block_t*)addr;

        /* 🔥 Check if FREE using safe scan */
        uint8_t is_free = 0;
        block_t* temp = free_list;

        while (temp)
        {
            if (temp == block)
            {
                is_free = 1;
                break;
            }
            temp = temp->next;
        }

        /* DRAW */
        if (is_free)
            oled_pixel(x, y);        // FREE → light pixel
        else
            oled_pixel(x, y);        // USED → same (OLED is mono)

        addr += sizeof(block_t) + block->size;

        x++;
        if (x >= 128)
        {
            x = 0;
            y++;
            if (y >= 6) break;
        }
    }

    /* ===== DEBUG TEXT ===== */
    oled_set_cursor(0, 6);

    oled_print("U:");
    oled_print_num(heap_used());

    oled_print(" S:");
    oled_print_num(heap_size());

    oled_set_cursor(0, 7);

    oled_print("Leak:");
    if (heap_has_leak())
        oled_print("YES");
    else
        oled_print("NO");

    oled_update();
}

/* ================== MAIN ================== */
int main(void)
{
    i2c_init();
    oled_init();

    void* p1 = my_malloc(1);
void* p2 = my_malloc(1);

draw_heap_map();

my_free(p1);
my_free(p2);
    while (1)
    {
        draw_heap_map();
        _delay_ms(1000);
    }
}