#include "cap.h"

Arena arena_create(u64 block_size, Arena* allocator) {
    Arena arena = {0};

    u64 allocation_size = sizeof(Arena_Block) + block_size;

    u8* allocation;
    if (allocator != NULL) {
        allocation = arena_alloc(allocator, allocation_size);
    } else {
        allocation = malloc(allocation_size);
        if (allocation == NULL) {
            mabort(str("Failed to allocate arena. malloc failed."));
        }
    }

    Arena_Block* block = (Arena_Block*)allocation;
    allocation += sizeof(Arena_Block);

    block->next = NULL;
    block->front = allocation;
    block->end = allocation + block_size;
    block->current = allocation;

    arena.current = block;
    arena.head = block;
    arena.allocator = allocator;

    return arena;
}

void* arena_alloc(Arena* arena, u64 size) {
    Arena_Block* block = arena->current;
    u8* data = block->current;
    block->current += size;

    if (block->current > block->end) {
        u64 last_arena_capacity = block->end - block->front;
        u64 new_arena_capacity = last_arena_capacity * 2;

        u8* allocation;
        if (arena->allocator != NULL) {
            allocation = arena_alloc(arena->allocator, new_arena_capacity);
        } else {
            allocation = malloc(new_arena_capacity);
            if (allocation == NULL) {
                mabort(str("Arena failed to get more memory. malloc failed."));
            }
        }

        Arena_Block* new_block = (Arena_Block*)allocation;
        allocation += sizeof(Arena_Block);
        block->next = new_block;
        new_block->next = NULL;
        new_block->front = allocation;
        new_block->end = allocation + new_arena_capacity;
        new_block->current = allocation;

        arena->current = new_block;
    }

    return data;
}

void arena_clear(Arena* arena) {
    Arena_Block* block = arena->head;
    while (block != NULL) {
        block->current = block->front;
        block = block->next;
    }
    arena->current = arena->head;
}

void arena_free(Arena* arena) {
    if (arena->allocator != NULL) return;
    Arena_Block* block = arena->head->next;
    while (block != NULL) {
        Arena_Block* next = block->next;
        free(block);
    }
    free(arena);
}
