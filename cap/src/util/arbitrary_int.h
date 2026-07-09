#pragma once
#include "base.h"

void* arbitrary_int_cast(void* value, u64 current_bits, u64 new_bits, bool is_signed);

f64 arbitrary_int_cast_to_float(void* value, u64 current_bits, bool is_signed);
