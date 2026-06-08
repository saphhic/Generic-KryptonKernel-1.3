// written by saphhic.

#include "kmalloc.h"

#define HEAP_SIZE (4 * 1024 * 1024)

#define ALIGN 8
#define ALIGN_MASK (ALIGN - 1)

struct BlockHeader {
    uint32_t size;
    uint32_t used;
};

#define HEADER_SIZE sizeof(BlockHeader)

static uint8_t heap_arena[HEAP_SIZE];

static uint8_t* heap_start = heap_arena;
static uint8_t* heap_top = heap_arena;
static uint8_t* heap_end = heap_arena + HEAP_SIZE;

static uint32_t stat_allocs = 0;
static uint32_t stat_frees = 0;
static uint32_t stat_bytes = 0;

static inline uint32_t align_up(uint32_t n) {
    return (n + ALIGN_MASK) & ~ALIGN_MASK;
}

static inline BlockHeader* next_block(BlockHeader* hdr) {
    return (BlockHeader*)((uint8_t*)hdr + HEADER_SIZE + hdr->size);
}

void* kmalloc(uint32_t size) {
    if (size == 0) {
        return nullptr;
    }

    uint32_t aligned = align_up(size);

    uint8_t* cursor = heap_start;
    while (cursor + HEADER_SIZE <= heap_top) {
        BlockHeader* hdr = (BlockHeader*)cursor;
        if (!hdr->used && hdr->size >= aligned) {
            if (hdr->size >= aligned + HEADER_SIZE * ALIGN) {
                uint32_t leftover = hdr->size - aligned - HEADER_SIZE;
                hdr->size = aligned;
                BlockHeader* split = next_block(hdr);
                split->size = leftover;
                split->used = 0;
            }
            hdr->used = 1;
            stat_allocs++;
            stat_bytes += hdr->size;
            return (void*)((uint8_t*)hdr + HEADER_SIZE);
        }
        cursor += HEADER_SIZE + hdr->size;
    }
    uint32_t needed = HEADER_SIZE + aligned;
    if (heap_top + needed > heap_end) {
        return nullptr;
    }
    BlockHeader* hdr = (BlockHeader*)heap_top;
    hdr->size = aligned;
    hdr->used = 1;
    heap_top += needed;
    stat_allocs++;
    stat_bytes += hdr->size;
    return (void*)((uint8_t*)hdr + HEADER_SIZE);
}

void* kcalloc(uint32_t count, uint32_t size) {
    if (size != 0 && count > UINT32_MAX / size) {
        return nullptr;
    }
    uint32_t total = count * size;
    void* ptr = kmalloc(total);
    if (!ptr) {
        return nullptr;
    }
    uint8_t* p = (uint8_t*)ptr;
    for (uint32_t i = 0; i < total; i++) {
        p[i] = 0;
    }
    return ptr;
}

void kfree(void* ptr) {
    if (!ptr) return;
    BlockHeader* hdr = (BlockHeader*)((uint8_t*)ptr - HEADER_SIZE);
    if (!hdr->used) return;
    hdr->used = 0;
    stat_frees++;
    if (stat_bytes >= hdr->size) stat_bytes -= hdr->size;
    if ((uint8_t*)next_block(hdr) < heap_top) {
        BlockHeader* next = next_block(hdr);
        if (!next->used) {
            hdr->size += HEADER_SIZE + next->size;
        }
    }
}

void* krealloc(void* ptr, uint32_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (!new_size) {
        kfree(ptr);
        return nullptr;
    }
    BlockHeader* hdr = (BlockHeader*)((uint8_t*)ptr - HEADER_SIZE);

    uint32_t aligned = align_up(new_size);
    if (hdr->size >= aligned) return ptr;
    
    void* new_ptr = kmalloc(new_size);
    if (!new_ptr) return nullptr;

    uint8_t* src = (uint8_t*)ptr;
    uint8_t* dst = (uint8_t*)new_ptr;
    uint32_t copy_size = hdr->size < aligned ? hdr->size : aligned;
    for (uint32_t i = 0; i < copy_size; i++) {
        dst[i] = src[i];
    }
    kfree(ptr);
    return new_ptr;
}

char* kstrdup(const char* str) {
    if (!str) return nullptr;
    uint32_t len = 0;
    while (str[len]) len++;
    char* copy = (char*)kmalloc(len + 1);
    if (!copy) return nullptr;
    for (uint32_t i = 0; i <= len; i++) {
        copy[i] = str[i];
    }
    return copy;
}

char* kstrndup(const char* str, uint32_t n) {
    if (!str) return nullptr;
    uint32_t len = 0;
    while (len < n && str[len]) len++;
    char* copy = (char*)kmalloc(len + 1);
    if (!copy) return nullptr;
    for (uint32_t i = 0; i < len; i++) {
        copy[i] = str[i];
    }
    copy[len] = '\0';
    return copy;
}

void heap_stats(uint32_t* out_used, uint32_t* out_free, uint32_t* out_allocs) {
    if (out_used) *out_used = stat_bytes;
    if (out_free) *out_free = (uint32_t)(heap_end - heap_top) + (stat_allocs - stat_frees) * 0;    
    if (out_allocs) *out_allocs = stat_allocs;

    uint32_t free_bytes = 0;
    uint8_t* cursor = heap_start;
    while (cursor + HEADER_SIZE <= heap_top) {
        BlockHeader* hdr = (BlockHeader*)cursor;
        if (!hdr->used) {
            free_bytes += hdr->size;
        }
        cursor += HEADER_SIZE + hdr->size;
    }
    free_bytes += (uint32_t)(heap_end - heap_top);
    if (out_free) *out_free = free_bytes;
}
