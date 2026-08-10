#include "base.h"

char* read_file(const char* path) {
    FILE* file = NULL;
    errno_t err = fopen_s(&file, path, "rb");
    if (err != 0 || file == NULL) return NULL;
    fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    rewind(file);
    char* data = alloc(size + 2);
    fread(data, 1, size, file);
    data[size] = '\n';
    data[size + 1] = 0;
    fclose(file);
    return data;
}

#if defined(_WIN32)
#include <windows.h>
f64 get_time_in_seconds(void) {
    static LARGE_INTEGER frequency = {0};
    LARGE_INTEGER time;
    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }
    QueryPerformanceCounter(&time);
    return (f64)time.QuadPart / (f64)frequency.QuadPart;
}
#else
#include <time.h>
f64 get_time_in_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1000000000.0;
}
#endif

bool bit_get(const void* data, u32 bit_index) {
    u32 byte_index = bit_index / 8;
    u32 bit_offset = bit_index % 8;
    u8 byte = ((const u8*)data)[byte_index];
    return ((byte >> bit_offset) & 1u) != 0;
}

void bit_set(void* data, u32 bit_index, bool value) {
    u32 byte_index = bit_index / 8;
    u32 bit_offset = bit_index % 8;
    u8* byte = &((u8*)data)[byte_index];
    if (value) {
        *byte |= (u8)(1u << bit_offset);
    } else {
        *byte &= (u8) ~(u8)(1u << bit_offset);
    }
}
