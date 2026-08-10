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

utf8 utf8_slice(char* start, char* end) {
    u32 len = end - start;
    utf8 str = {0};
    str.data = start;
    str.count = len;
    return str;
}

utf8 utf8_memory_as_hex(void* memory, u64 size) {
    utf8 str = {0};
    str.data = alloc(size * 2 + 3);
    str.count = 2;
    str.data[0] = '0';
    str.data[1] = 'x';
    for (i64 i = size - 1; i >= 0; i--) {
        char* c = str.data + str.count;
        sprintf(c, "%02x", *(u8*)(memory + i));
        str.count += 2;
    }
    return str;
}

void utf8_append_with_capacity(utf8* base, u32* capacity, utf8 str) {
    i64 amount_over_capacity = (str.count + base->count) - (*capacity);
    if (amount_over_capacity >= 0) {
        u32 capacity2 = *capacity * 2;
        if (amount_over_capacity > capacity2) {
            capacity2 = capacity2 + amount_over_capacity * 2;
        }
        char* new_memory = alloc(capacity2);
        memcpy(new_memory, base->data, base->count);
        base->data = new_memory;
        *capacity = capacity2;
    }
    memcpy(base->data + base->count, str.data, str.count);
    base->count += str.count;
}

utf8 utf8_append(utf8 base, utf8 str) {
    i64 new_size = str.count + base.count;
    void* new_memory = alloc(new_size);
    memcpy(new_memory, base.data, base.count);
    memcpy(((char*)new_memory) + base.count, str.data, str.count);
    utf8 new_str = {0};
    new_str.data = new_memory;
    new_str.count = new_size;
    return new_str;
}

void utf8_builder_append(utf8_builder* builder, utf8 str) {
    i64 amount_over_capacity = (str.count + builder->str.count) - (builder->capacity);
    if (amount_over_capacity >= 0) {
        u32 capacity2 = builder->capacity * 2;
        if (amount_over_capacity > capacity2) {
            capacity2 = capacity2 + amount_over_capacity * 2;
        }
        char* new_memory = alloc(capacity2);
        memcpy(new_memory, builder->str.data, builder->str.count);
        builder->str.data = new_memory;
        builder->capacity = capacity2;
    }
    memcpy(builder->str.data + builder->str.count, str.data, str.count);
    builder->str.count += str.count;
}
