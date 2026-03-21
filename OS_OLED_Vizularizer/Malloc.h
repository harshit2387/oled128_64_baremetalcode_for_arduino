#ifndef MALLOC_H
#define MALLOC_H

#include <stdint.h>
#include <avr/io.h>

/* ===== LINKER SYMBOLS ===== */
extern char __bss_end;
extern char *__brkval;

/* ===== STACK POINTER ===== */
#define STACK_POINTER ((uint16_t)SP)

/* ===== BLOCK STRUCT ===== */
typedef struct block
{
    uint16_t size;
    struct block* next;
} block_t;

/* ===== GLOBALS ===== */
static block_t* free_list = 0;

/* ===== DEBUG ===== */
static uint16_t malloc_count = 0;
static uint16_t free_count   = 0;
static uint16_t current_alloc = 0;

/* ================== MALLOC ================== */
static void* my_malloc(uint16_t size)
{
    if (size == 0) return 0;

    /* ALIGN */
    if (size & 1) size++;

    block_t* prev = 0;
    block_t* curr = free_list;

    /* ===== FIRST-FIT ===== */
    while (curr)
    {
        if (curr->size >= size)
        {
            /* REMOVE FROM FREE LIST */
            if (prev)
                prev->next = curr->next;
            else
                free_list = curr->next;

            /* 🔥 IMPORTANT FIX: preserve correct size */
            uint16_t actual_size = curr->size;

            malloc_count++;
            current_alloc += actual_size;

            return (void*)(curr + 1);
        }

        prev = curr;
        curr = curr->next;
    }

    /* ===== EXTEND HEAP ===== */
    uint16_t heap_end;

    if (__brkval == 0)
        heap_end = (uint16_t)&__bss_end;
    else
        heap_end = (uint16_t)__brkval;

    uint16_t total_size = size + sizeof(block_t);

    /* STACK COLLISION CHECK */
    if (heap_end + total_size >= STACK_POINTER - 32)
        return 0;

    block_t* new_block = (block_t*)heap_end;
    new_block->size = size;

    __brkval = (char*)(heap_end + total_size);

    malloc_count++;
    current_alloc += size;

    return (void*)(new_block + 1);
}

/* ================== FREE ================== */
static void my_free(void* ptr)
{
    if (!ptr) return;

    block_t* block = ((block_t*)ptr) - 1;

    free_count++;
    current_alloc -= block->size;

    /* ===== INSERT SORTED ===== */
    if (!free_list || block < free_list)
    {
        block->next = free_list;
        free_list = block;
    }
    else
    {
        block_t* curr = free_list;

        while (curr->next && curr->next < block)
            curr = curr->next;

        block->next = curr->next;
        curr->next = block;
    }

    /* ===== COALESCE ===== */
    block_t* curr = free_list;

    while (curr && curr->next)
    {
        uint16_t curr_end =
            (uint16_t)curr + sizeof(block_t) + curr->size;

        if (curr_end == (uint16_t)(curr->next))
        {
            curr->size += sizeof(block_t) + curr->next->size;
            curr->next = curr->next->next;
        }
        else
        {
            curr = curr->next;
        }
    }
}

/* ================== DEBUG ================== */
/* 🔥 Total heap used */
static uint16_t heap_used(void)
{
    return current_alloc;
}

/* 🔥 Heap size */
static uint16_t heap_size(void)
{
    if (__brkval == 0)
        return 0;

    return (uint16_t)__brkval - (uint16_t)&__bss_end;
}

/* 🔥 Largest free block */
static uint16_t largest_free_block(void)
{
    block_t* curr = free_list;
    uint16_t max = 0;

    while (curr)
    {
        if (curr->size > max)
            max = curr->size;
        curr = curr->next;
    }

    return max;
}

/* 🔥 Fragmentation % */
static uint8_t heap_fragmentation(void)
{
    uint16_t total_free = heap_size() - heap_used();

    if (total_free == 0) return 0;

    uint16_t largest = largest_free_block();

    return (uint8_t)(100 - (largest * 100) / total_free);
}

/* 🔥 Leak Detection */
static uint8_t heap_has_leak(void)
{
    return (malloc_count != free_count);
}

#endif