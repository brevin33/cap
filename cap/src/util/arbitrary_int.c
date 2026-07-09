#include "util.h"

void* arbitrary_int_cast(void* value, u64 current_bits, u64 new_bits, bool is_signed) {
    u64 new_size = (new_bits + 7) / 8;
    void* result = alloc(new_size);
    u64 current_size = (current_bits + 7) / 8;
    u64 copy_size = current_size < new_size ? current_size : new_size;
    memcpy(result, value, copy_size);

    // Extract the true sign bit at bit position (current_bits - 1)
    u64 sign_bit_byte_idx = (current_bits - 1) / 8;
    u64 sign_bit_pos = (current_bits - 1) % 8;
    u8 sign_bit = (((u8*)value)[sign_bit_byte_idx] >> sign_bit_pos) & 0x1;

    if (is_signed && new_size > current_size) {
        u8 fill = sign_bit ? 0xFF : 0x00;
        memset((u8*)result + current_size, fill, new_size - current_size);
    }

    // Mask off unused bits in the last byte of result
    if (new_bits % 8 != 0) {
        u64 last_byte_idx = new_size - 1;
        u8 mask = (1 << (new_bits % 8)) - 1;
        ((u8*)result)[last_byte_idx] &= mask;
    }

    return result;
}

f64 arbitrary_int_cast_to_float(void* value, u64 current_bits, bool is_signed) {
    f64 result = 0.0;
    u64 byte_count = (current_bits + 7) / 8;

    if (is_signed) {
        // Check sign bit
        u8 sign_byte_idx = (current_bits - 1) / 8;
        u8 sign_bit_idx = (current_bits - 1) % 8;
        bool is_negative = (((u8*)value)[sign_byte_idx] >> sign_bit_idx) & 1;

        if (is_negative) {
            // Two's complement: negate and make positive
            u8 temp[256];
            memcpy(temp, value, byte_count);

            // Invert all bits up to current_bits
            for (u64 i = 0; i < byte_count; i++) {
                temp[i] = ~temp[i];
            }
            // Clear bits beyond current_bits
            u8 last_byte_bits = current_bits % 8;
            if (last_byte_bits) {
                temp[byte_count - 1] &= (1 << last_byte_bits) - 1;
            }

            // Add 1
            u64 carry = 1;
            for (u64 i = 0; i < byte_count && carry; i++) {
                u16 sum = (u16)temp[i] + carry;
                temp[i] = sum & 0xFF;
                carry = sum >> 8;
            }

            // Convert to f64 and negate
            result = 0.0;
            for (u64 i = byte_count; i > 0; i--) {
                result = result * 256.0 + (f64)temp[i - 1];
            }
            result = -result;
        } else {
            // Positive: convert directly
            result = 0.0;
            for (u64 i = byte_count; i > 0; i--) {
                result = result * 256.0 + (f64)((u8*)value)[i - 1];
            }
        }
    } else {
        // Unsigned: convert directly
        result = 0.0;
        for (u64 i = byte_count; i > 0; i--) {
            result = result * 256.0 + (f64)((u8*)value)[i - 1];
        }
    }

    return result;
}
