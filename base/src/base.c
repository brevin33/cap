#include "base.h"

Context context = {0};

void init_base_context() {
    context.arena = arena_create_root(1024 * 1024);
}
