#pragma once

#include "base.h"

typedef struct utf8 utf8;

struct utf8 {
    char* data;
    u64 count;
};

#define utf8_fmt(str) ((int)(str).count), ((str).data)
#define utf8_str(str) ((utf8){.data = ((char*)(u64)(str)), .count = (strlen((str)))})

// alters utf8 string by going to the next character
// most efficient way to iterate over utf8 string
void utf8_next(utf8* str);

u32 utf8_get(utf8 str);

bool utf8_equal(utf8 a, utf8 b);

// inefficient way to iterate over utf8 string
u32 utf8_index(utf8 str, u32 index);

char* utf8_to_str(utf8 str);

u32 utf8_visual_len(utf8 str);

void utf8_append_with_capacity(utf8* base, u32 base_capacity, utf8 str);
