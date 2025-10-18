/* Minimal Memory Management Header */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

void memory_init(void);
void* malloc(size_t size);
void free(void* ptr);

#endif
