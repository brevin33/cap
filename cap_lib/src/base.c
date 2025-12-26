#include "cap.h"

Cap_Context cap_context;

void* cap_alloc(u64 size) {
    Arena* arena = &cap_context.arena;
    return arena_alloc(arena, size);
}

void cap_init() { cap_context.arena = arena_create(1024 * 1024, NULL); }
