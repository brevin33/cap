#include "util.h"

void utf8_next(utf8* str) {
    if (str->count == 0) return;
    char* c = str->data;
    u32 len;
    if ((*c & 0x80) == 0x00) len = 1;
    else if ((*c & 0xE0) == 0xC0) len = 2;
    else if ((*c & 0xF0) == 0xE0) len = 3;
    else if ((*c & 0xF8) == 0xF0) len = 4;
    else len = 1;
    if (str->count < len) return;
    u32 val = *(u32*)c & ((1u << (len * 8)) - 1);
    str->data += len;
    str->count -= len;
}

bool utf8_equal(utf8 a, utf8 b) {
    if (a.count != b.count) return false;
    return memcmp(a.data, b.data, a.count) == 0;
}

u32 utf8_get(utf8 str) {
    char* c = str.data;
    u32 len;
    if ((*c & 0x80) == 0x00) len = 1;
    else if ((*c & 0xE0) == 0xC0) len = 2;
    else if ((*c & 0xF0) == 0xE0) len = 3;
    else if ((*c & 0xF8) == 0xF0) len = 4;
    else len = 1;
    if (str.count < len) return 0;
    u32 val = *(u32*)c & ((1u << (len * 8)) - 1);
    return val;
}

u32 utf8_index(utf8 str, u32 index) {
    if (index == 0) return utf8_get(str);
    char* c = str.data;
    while (true) {
        utf8_next(&str);
        index--;
        if (index == 0) return utf8_get(str);
        if (str.count == 0) return 0;
    }
}

char* utf8_to_str(utf8 str) {
    char* res = alloc(str.count + 1);
    memcpy(res, str.data, str.count);
    res[str.count] = 0;
    return res;
}

u32 utf8_visual_len(utf8 str) {
    u32 length = 0;
    for (size_t i = 0; i < str.count; i++) {
        if (str.data[i] == '\x1b') {  // Start of escape sequence
            while (i < str.count && str.data[i] != 'm') i++;
        } else {
            // Basic check for start of UTF-8 character (0xxxxxxx or 11xxxxxx)
            if ((str.data[i] & 0xC0) != 0x80) length++;
        }
    }
    return length;
}

void utf8_append_with_capacity(utf8* base, u32 base_capacity, utf8 str) {
    i64 amount_over_capacity = (str.count + base->count) - base_capacity;
    if (amount_over_capacity < 0) amount_over_capacity = 0;
    u32 copy_amount = str.count - amount_over_capacity;
    memcpy(base->data + base->count, str.data, copy_amount);
    base->count += copy_amount;
}
