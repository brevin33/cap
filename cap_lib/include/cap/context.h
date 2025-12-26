#pragma once

#include "cap/arena.h"
#include "cap/base.h"

typedef struct Arena Arena;
typedef struct Cap_Context Cap_Context;

struct Cap_Context {
    Arena arena;
};

extern Cap_Context cap_context;
