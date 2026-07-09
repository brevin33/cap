#pragma once

#include "base.h"

typedef struct Arena Arena;
typedef struct Arena_Block Arena_Block;

struct Arena_Block {
    Arena_Block* next;
    u64 size;
    u64 used;
    void* data;
};

struct Arena {
    Arena* parent;
    Arena_Block* head;
    Arena_Block* current;
};

Arena arena_init(u64 size, Arena* parent);

void* arena_alloc(Arena* arena, u64 size);

void arena_clear(Arena* arena);

void arena_free(Arena* arena);
