#pragma once
#include "basic.h"

typedef struct Arena Arena;
typedef struct Arena {
    void** data;
    u64 count;
    u64 capacity;
    u64 index;
    u64 initial_size;
    Arena* parent;
} Arena;

Arena arena_create_root(u64 initial_size);
Arena arena_create(Arena* parent, u64 initial_size);
void* arena_alloc(Arena* arena, u64 size);
void arena_free(Arena* arena);
void arena_reset(Arena* arena);
