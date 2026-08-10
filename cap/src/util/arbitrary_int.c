#include "util.h"

void* arbitrary_int_cast(void* value, u64 current_bits, u64 new_bits, bool is_signed) {
    u64 new_size_bytes = (new_bits + 7) / 8;
    void* result = alloc(new_size_bytes);
    bool is_negative = is_signed && bit_get(value, current_bits - 1);
    u64 smallest_bits = current_bits < new_bits ? current_bits : new_bits;
    for (u64 i = 0; i < smallest_bits; i++) {
        bool bit = bit_get(value, i);
        bit_set(result, i, bit);
    }
    for (u64 i = current_bits; i < new_bits; i++) {
        bit_set(result, i, is_negative);
    }
    bit_set(result, new_bits - 1, is_negative);
    for (u64 i = new_bits; i < new_size_bytes * 8; i++) {
        bit_set(result, i, is_negative);
    }
    return result;
}

utf8 arbitrary_int_to_string(void* value, u64 bits, bool is_signed) {
    bool is_negative = is_signed && bit_get(value, bits - 1);
    u64 digit_count = 0;
    u64 digit_capacity = 0;
    char* digits = NULL;

    void* temp = alloc((bits + 7) / 8);
    memcpy(temp, value, (bits + 7) / 8);

    if (is_negative) {
        void* negated = temp;
        for (u64 i = 0; i < bits; i++) {
            bit_set(negated, i, !bit_get(negated, i));
        }
        bool carry = true;
        for (u64 i = 0; i < bits && carry; i++) {
            bool bit = bit_get(negated, i);
            bit_set(negated, i, !bit);
            carry = bit;
        }
        memcpy(temp, negated, (bits + 7) / 8);
    }

    bool is_zero = true;
    for (u64 i = 0; i < bits; i++) {
        if (bit_get(temp, i)) {
            is_zero = false;
            break;
        }
    }
    if (is_zero) return utf8_str("0");
    while (!is_zero) {
        u64 remainder = 0;
        for (i64 i = bits - 1; i >= 0; i--) {
            u64 bit = bit_get(temp, i) ? 1 : 0;
            remainder = (remainder << 1) | bit;
            bit_set(temp, i, (remainder / 10) & 1);
            remainder %= 10;
        }
        char digit = '0' + remainder;
        ptr_append(digits, digit_count, digit_capacity, digit);
        is_zero = true;
        for (u64 i = 0; i < bits; i++) {
            if (bit_get(temp, i)) {
                is_zero = false;
                break;
            }
        }
    }

    utf8 result = {0};
    result.data = alloc(digit_count + 1);
    result.count = digit_count + is_negative;
    char* buffer = result.data;
    if (is_negative) {
        buffer[0] = '-';
        for (u64 i = 0; i < digit_count; i++) {
            buffer[i + 1] = digits[digit_count - 1 - i];
        }
    } else {
        for (u64 i = 0; i < digit_count; i++) {
            buffer[i] = digits[digit_count - 1 - i];
        }
    }
    return result;
}

f64 arbitrary_int_cast_to_float(void* value, u64 bits, bool is_signed) {
    f64 result;
    bool last_bit_set = bit_get(value, bits - 1);
    if (is_signed) {
        result = -last_bit_set;
    } else {
        result = last_bit_set;
    }
    for (i64 i = bits - 2; i >= 0; i--) {
        bool bit = bit_get(value, i);
        result = result * 2.0 + bit;
    }
    return result;
}
