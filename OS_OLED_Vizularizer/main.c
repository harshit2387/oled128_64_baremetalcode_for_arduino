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
        oled_set_cursor(0, 0);
        oled_print("OOM!");
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
        return;
    }

    OLED_Node* temp = head;
    while (temp->next)
        temp = temp->next;

    temp->next = new_node;
}

/* ================== RENDER PIXELS ================== */
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
    uint16_t heap_end = (__brkval == 0) ? heap_start : (uint16_t)__brkval;

    uint16_t addr = heap_start;
    uint8_t x = 0, y = 0;

    while (addr + sizeof(block_t) < heap_end)
    {
        block_t* block = (block_t*)addr;

        /* ===== SAFETY CHECK ===== */
        if (block->size == 0 || block->size > 1024)
            break;

        /* ===== CHECK FREE ===== */
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

        /* ===== WIDTH BASED ON SIZE ===== */
        uint8_t width = block->size / 8;
        if (width == 0) width = 1;

        for (uint8_t i = 0; i < width && x < 128; i++)
        {
            if (is_free)
            {
                oled_pixel(x, y);          // FREE
            }
            else
            {
                oled_pixel(x, y);          // USED (thicker)
                if (y < 7) oled_pixel(x, y + 1);
            }
            x++;
        }

        addr += sizeof(block_t) + block->size;

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

    oled_print("F:");
    oled_print_num(heap_fragmentation());
    oled_print("% ");

    oled_print("L:");
    if (heap_has_leak())
        oled_print("Y");
    else
        oled_print("N");

    oled_update();
}

/* ================== MAIN ================== */
int main(void)
{
    i2c_init();
    oled_init();

    /* ===== TEST ALLOCATIONS ===== */
    void* p1 = my_malloc(10);
    void* p2 = my_malloc(20);
    void* p3 = my_malloc(50);
    void* p4 = my_malloc(5);
    void* p5 = my_malloc(80);

    draw_heap_map();
    _delay_ms(1500);

    /* ===== FREE SOME (CREATE FRAGMENTATION) ===== */
    my_free(p2);
    my_free(p4);

    draw_heap_map();
    _delay_ms(1500);

    /* ===== MORE ALLOCATIONS ===== */
    void* p6 = my_malloc(15);
    void* p7 = my_malloc(25);

    (void)p1;
    (void)p3;
    (void)p5;
    (void)p6;
    (void)p7;

    /* ===== LOOP ===== */
    while (1)
    {
        draw_heap_map();
        _delay_ms(1000);
    }
}