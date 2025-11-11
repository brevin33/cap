#include "base.h"

static void* arena_inside_alloc(Arena* arena, u64 size) {
    if (arena->parent) {
        return arena_alloc(arena->parent, size);
    } else {
        return malloc(size);
    }
}

static void arena_inside_free(Arena* arena, void* data) {
    if (arena->parent) {
        // do nothing
    } else {
        free(data);
    }
}

static u64 arena_index_allocation_size(Arena* arena, u64 index) {
    u64 size = arena->initial_size;
    for (u64 i = 0; i < index; i++) {
        size *= 2;
    }
    return size;
}

Arena arena_create_root(u64 initial_size) {
    return arena_create(NULL, initial_size);
}
Arena arena_create(Arena* parent, u64 initial_size) {
    Arena arena;
    arena.parent = parent;
    arena.count = 0;
    arena.capacity = 8;
    arena.data = arena_inside_alloc(&arena, arena.capacity * sizeof(void*));
    memset(arena.data, 0, arena.capacity * sizeof(void*));
    arena.data[0] = arena_inside_alloc(&arena, initial_size);
    arena.index = 0;
    arena.initial_size = initial_size;
    return arena;
}
void* arena_alloc(Arena* arena, u64 size) {
    size = (size + 7) & ~7;
    u64 allocation_size = arena_index_allocation_size(arena, arena->count);
    while (allocation_size <= arena->index + size) {
        arena->count++;
        if (arena->count >= arena->capacity) {
            u64 old_capacity = arena->capacity;
            arena->capacity *= 2;
            void* old_data = arena->data;
            arena->data = arena_inside_alloc(arena, arena->capacity * sizeof(void*));
            memset(arena->data, 0, arena->capacity * sizeof(void*));
            memcpy(arena->data, old_data, old_capacity * sizeof(void*));
            arena_inside_free(arena, old_data);
        }
        allocation_size = arena_index_allocation_size(arena, arena->count);
        if (arena->data[arena->count] == NULL) {
            arena->data[arena->count] = arena_inside_alloc(arena, allocation_size);
        }
    }
    void* data = arena->data[arena->count] + arena->index;
    arena->index += size;
    memset(data, 0, size);
    return data;
}

void arena_free(Arena* arena) {
    for (u64 i = 0; i < arena->count; i++) {
        arena_inside_free(arena, arena->data[i]);
    }
    arena_inside_free(arena, arena->data);
}

void arena_reset(Arena* arena) {
    arena->index = 0;
    arena->count = 0;
}
