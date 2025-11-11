#pragma once
#include "base/arena.h"
#include "base/basic.h"
#include "base/list.h"

typedef struct Context {
    Arena arena;
} Context;

extern Context context;

void init_base_context();
