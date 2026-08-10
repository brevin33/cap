#pragma once

#include "base.h"

typedef struct utf8 utf8;
typedef struct utf8_builder utf8_builder;

struct utf8 {
    char* data;
    u64 count;
};

struct utf8_builder {
    utf8 str;
    u64 capacity;
};

#define utf8_fmt(str) ((int)(str).count), ((str).data)
#define utf8_str(str) ((utf8){.data = ((char*)(u64)(str)), .count = (strlen((str)))})
#define utf8_builder(utf8) ((utf8_builder){.str = (utf8), .capacity = (utf8).count})

// alters utf8 string by going to the next character
// most efficient way to iterate over utf8 string
void utf8_next(utf8* str);

u32 utf8_get(utf8 str);

bool utf8_equal(utf8 a, utf8 b);

// inefficient way to iterate over utf8 string
u32 utf8_index(utf8 str, u32 index);

char* utf8_to_str(utf8 str);

u32 utf8_visual_len(utf8 str);

utf8 utf8_append(utf8 base, utf8 str);

void utf8_builder_append(utf8_builder* builder, utf8 str);

utf8 utf8_slice(char* start, char* end);

utf8 utf8_memory_as_hex(void* memory, u64 size);
