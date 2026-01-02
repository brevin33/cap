#pragma once

#include "cap/base.h"

typedef struct String String;

struct String {
    char* data;
    u64 length;
};

#define str(s) ((String){.data = ((char*)(u64)(s)), .length = sizeof((s)) - 1})

#define str_info(s) (int)((s).length), ((s).data)

bool string_equal(String a, String b);

String string_create(char* data, u64 length);

String string_append(String a, String b);

String string_int(i64 value);

String string_float(f64 value);
