//written by saphhic.

#pragma once
#include <stdint.h>

void* kmalloc(uint32_t size);
void* kcalloc(uint32_t count, uint32_t size);
void* krealloc(void* ptr, uint32_t new_size);
void kfree(void* ptr);

char* kstrdup(const char* str);
char* kstrndup(const char* str, uint32_t n);

void heap_stats(uint32_t* out_used, uint32_t* out_free, uint32_t* out_allocs);