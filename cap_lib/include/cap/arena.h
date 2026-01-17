#pragma once

#include "cap/base.h"

typedef struct Arena Arena;
typedef struct Arena_Block Arena_Block;

struct Arena_Block {
    Arena_Block* next;
    u8* front;
    u8* end;
    u8* current;
};

struct Arena {
    Arena_Block* current;
    Arena_Block* head;
    Arena* allocator;
};

Arena arena_create(u64 block_size, Arena* allocator);

void* arena_alloc(Arena* arena, u64 size);

void arena_clear(Arena* arena);

void arena_free(Arena* arena);

bool arena_memory_is_inside_arena(Arena* arena, void* memory);
