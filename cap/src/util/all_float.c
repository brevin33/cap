#include "util.h"

void* all_float_cast(void* value, u64 current_bits, u64 new_bits) {
    if (new_bits == current_bits) {
        u64 size = (new_bits + 7) / 8;
        void* new_value = alloc(size);
        memcpy(new_value, value, size);
        return new_value;
    }
    switch (current_bits) {
        case 16: {
            abort();
        }
        case 32: {
            f32* float_ = value;
            switch (new_bits) {
                case 16: {
                    abort();
                }
                case 64: {
                    f64* result = alloc(sizeof(f64));
                    *result = (f64)*float_;
                    return result;
                }
                case 128: {
                    abort();
                }
            }
        }
        case 64: {
            f64* float_ = value;
            switch (new_bits) {
                case 16: {
                    abort();
                }
                case 32: {
                    f32* result = alloc(sizeof(f32));
                    *result = (f32)*float_;
                    return result;
                }
                case 128: {
                    abort();
                }
                default: {
                    abort();
                }
            }
        }
        case 128: {
            abort();
        }
        default: {
            abort();
        }
    }
}
