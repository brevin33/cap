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
