#include "cap.h"

bool string_equal(String a, String b) {
    if (a.length != b.length) return false;
    for (u64 i = 0; i < a.length; i++) {
        if (a.data[i] != b.data[i]) return false;
    }
    return true;
}

String string_create(char* data, u64 length) { return (String){.data = data, .length = length}; }
