#ifndef MALLOC_H
#define MALLOC_H

#include <stdint.h>

/* Linker symbols */
extern char __bss_end;
extern char *__brkval;

/* Stack pointer */
#define STACK_POINTER ((uint16_t)SP)

/* ================== MALLOC ================== */
void* my_malloc(uint16_t size)
{
    if (size == 0) return 0;

    // Align to 2 bytes
    if (size & 1) size++;

    uint16_t heap_end;

    // If heap not used yet
    if (__brkval == 0)
        heap_end = (uint16_t)&__bss_end;
    else
        heap_end = (uint16_t)__brkval;

    // Check collision with stack
    if (heap_end + size >= STACK_POINTER - 32)
        return 0;   // Not enough memory

    void* ptr = (void*)heap_end;

    __brkval = (char*)(heap_end + size);

    return ptr;
}

void my_free(void* ptr)
{
    if (ptr == (void*)__brkval)
        __brkval = (char*)ptr;
}
#endif