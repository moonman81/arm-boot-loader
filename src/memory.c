/* Minimal Memory Management for Bootloader */

#include <stdint.h>
#include <stddef.h>
#include "memory.h"

// Simple heap - starts after bootloader code + stack
#define HEAP_START 0x00100000  // 1MB
#define HEAP_SIZE  0x00100000  // 1MB heap

typedef struct mem_block {
    size_t size;
    uint8_t free;
    struct mem_block *next;
} mem_block_t;

static mem_block_t *heap_start = NULL;

void memory_init(void) {
    heap_start = (mem_block_t*)HEAP_START;
    heap_start->size = HEAP_SIZE - sizeof(mem_block_t);
    heap_start->free = 1;
    heap_start->next = NULL;
}

void* malloc(size_t size) {
    if (!heap_start || size == 0) return NULL;

    // Align to 8-byte boundary
    size = (size + 7) & ~7;

    mem_block_t *current = heap_start;

    while (current) {
        if (current->free && current->size >= size) {
            // Split block if large enough
            if (current->size > size + sizeof(mem_block_t) + 8) {
                mem_block_t *new_block = (mem_block_t*)((uint8_t*)current + sizeof(mem_block_t) + size);
                new_block->size = current->size - size - sizeof(mem_block_t);
                new_block->free = 1;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }

            current->free = 0;
            return (void*)((uint8_t*)current + sizeof(mem_block_t));
        }
        current = current->next;
    }

    return NULL;
}

void free(void *ptr) {
    if (!ptr || !heap_start) return;

    mem_block_t *block = (mem_block_t*)((uint8_t*)ptr - sizeof(mem_block_t));
    block->free = 1;

    // Coalesce with next block if free
    if (block->next && block->next->free) {
        block->size += sizeof(mem_block_t) + block->next->size;
        block->next = block->next->next;
    }
}
