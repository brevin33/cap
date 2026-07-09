#include "arena.h"

void* _internal_arena_alloc(Arena* arena, u64 size) {
    if (arena->parent == NULL) {
        void* mem = malloc(size);
        if (mem == NULL) {
            printf("Malloc failed\n");
            exit(1);
        }
        return mem;
    } else {
        void* mem = arena_alloc(arena->parent, size);
        return mem;
    }
}

Arena_Block* _internal_alloc_block(Arena* arena, u64 size) {
    u8* mem = (u8*)_internal_arena_alloc(arena, sizeof(Arena_Block) + size);
    Arena_Block* block = (Arena_Block*)mem;
    block->size = size;
    block->data = mem + sizeof(Arena_Block);
    block->used = 0;
    block->next = NULL;
    return block;
}

Arena arena_init(u64 size, Arena* parent) {
    Arena arena = {};
    arena.parent = parent;
    Arena_Block* block = _internal_alloc_block(&arena, size);
    arena.head = block;
    arena.current = block;
    return arena;
}

void* arena_alloc(Arena* arena, u64 size) {
    size = (size + 7) & ~7;
    if (arena->current == NULL) {
        if (arena->head != NULL) arena->current = arena->head;
        else {
            Arena_Block* block = _internal_alloc_block(arena, size);
            arena->head = block;
            arena->current = block;
        }
    }
    while (arena->current->used + size > arena->current->size) {
        if (arena->current->next != NULL) {
            arena->current = arena->current->next;
        } else {
            u64 new_size = arena->current->size * 2;
            if (new_size < size) new_size = size;
            Arena_Block* new_block = _internal_alloc_block(arena, new_size);
            arena->current->next = new_block;
            arena->current = new_block;
        }
    }
    void* mem = arena->current->data + arena->current->used;
    arena->current->used += size;
    return mem;
}

void arena_clear(Arena* arena) {
    Arena_Block* block = arena->head;
    while (block != NULL) {
        block->used = 0;
        block = block->next;
    }
    arena->current = arena->head;
}

void arena_free(Arena* arena) {
    if (arena->parent != NULL) return;
    Arena_Block* block = arena->head;
    while (block != NULL) {
        Arena_Block* next = block->next;
        free(block);
        block = next;
    }
    arena->head = NULL;
    arena->current = NULL;
}
