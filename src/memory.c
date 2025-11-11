#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct block {
    size_t size;
    struct block* next;
    uint8_t free;
} block_t;

static uint8_t* heap_start = (uint8_t*)0x100000;
static uint8_t* heap_end   = (uint8_t*)0x200000;
static block_t* free_list  = NULL;

#define ALIGN4(x) (((x) + 3) & ~3)

void init_heap() {
    free_list = (block_t*)heap_start;
    free_list->size = heap_end - heap_start - sizeof(block_t);
    free_list->next = NULL;
    free_list->free = 1;
}

void* malloc(size_t size) {
    size = ALIGN4(size);
    block_t* curr = free_list;
    while (curr) {
        if (curr->free && curr->size >= size) {
            if (curr->size > size + sizeof(block_t)) {
                // podział bloku
                block_t* new_block = (block_t*)((uint8_t*)curr + sizeof(block_t) + size);
                new_block->size = curr->size - size - sizeof(block_t);
                new_block->free = 1;
                new_block->next = curr->next;
                curr->next = new_block;
                curr->size = size;
            }
            curr->free = 0;
            return (uint8_t*)curr + sizeof(block_t);
        }
        curr = curr->next;
    }
    return 0; // brak pamięci
}

void free(void* ptr) {
    if (!ptr) return;
    block_t* blk = (block_t*)((uint8_t*)ptr - sizeof(block_t));
    blk->free = 1;
    // można tu dodać łączenie sąsiadujących wolnych bloków
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    block_t* blk = (block_t*)((uint8_t*)ptr - sizeof(block_t));
    if (blk->size >= size) return ptr; // wystarczająco miejsca
    void* new_ptr = malloc(size);
    if (!new_ptr) return 0;
    // skopiuj stare dane
    uint8_t* src = ptr;
    uint8_t* dst = new_ptr;
    for (size_t i = 0; i < blk->size; i++)
        dst[i] = src[i];
    free(ptr);
    return new_ptr;
}
