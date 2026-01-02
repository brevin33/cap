#include "cap.h"

bool string_equal(String a, String b) {
    if (a.length != b.length) return false;
    for (u64 i = 0; i < a.length; i++) {
        if (a.data[i] != b.data[i]) return false;
    }
    return true;
}

String string_create(char* data, u64 length) {
    return (String){.data = data, .length = length};
}

String string_append(String a, String b) {
    String str = {0};
    str.data = cap_alloc(a.length + b.length + 1);
    str.length = a.length + b.length;
    memcpy(str.data, a.data, a.length);
    memcpy(str.data + a.length, b.data, b.length);
    return str;
}

String string_int(i64 value) {
    char buffer[1024];
    snprintf(buffer, 1024, "%lld", value);
    u64 length = strlen(buffer);
    char* ptr = cap_alloc(length + 1);
    memcpy(ptr, buffer, length);
    return string_create(ptr, length);
}

String string_float(f64 value) {
    char buffer[1024];
    snprintf(buffer, 1024, "%f", value);
    u64 length = strlen(buffer);
    char* ptr = cap_alloc(length + 1);
    memcpy(ptr, buffer, length);
    return string_create(ptr, length);
}
