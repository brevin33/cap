
#include "cap.h"

void init_cap() {
    cap_context.arena = arena_create(MB(16), NULL);
    cap_context.log = true;
}
