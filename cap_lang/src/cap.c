#include "cap.h"

Cap_Context cap_context = {0};

void init_cap_context() {
    cap_context.log_errors = true;
}
