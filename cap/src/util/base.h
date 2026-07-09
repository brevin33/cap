#include <direct.h>

#include "inttypes.h"
#include "signal.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#ifdef _WIN32
#include "win_dirent.h"
#else
#include "dirent.h"
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;

#define KB(x) ((x) * 1024)
#define MB(x) ((x) * 1024 * 1024)
#define GB(x) ((x) * 1024 * 1024 * 1024)

#define ptr_append(ptr, count, capacity, item)                     \
    do {                                                           \
        if ((count) >= (capacity)) {                               \
            (capacity) *= 2;                                       \
            if ((capacity) == 0) (capacity) = 8;                   \
            void* old_data = (ptr);                                \
            (ptr) = alloc((capacity) * sizeof((ptr)[0]));          \
            memcpy((ptr), (old_data), (count) * sizeof((ptr)[0])); \
        }                                                          \
        (ptr)[(count)++] = (item);                                 \
    } while (0)

#define arr_len(arr) (sizeof(arr) / sizeof(arr[0]))

#ifdef NDEBUG
#define massert(cond, msg)
#define debug_break() ((void)0)
#else
#if defined(_MSC_VER)
#define debug_break() __debugbreak()
#elif defined(__has_builtin)
#if __has_builtin(__builtin_trap)
#define debug_break() __builtin_trap()
#else
#define debug_break() ((void)0)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define debug_break() __builtin_trap()
#else
#define debug_break() ((void)0)
#endif
#endif

#ifndef NDEBUG
#define assert(cond)               \
    do {                           \
        if (!(cond)) {             \
            log_output(log_debug); \
            debug_break();         \
            exit(1);               \
        }                          \
    } while (0)
#define internal_compiler_error() \
    do {                          \
        log_output(log_debug);    \
        debug_break();            \
        exit(1);                  \
    } while (0)
#else
#define assert(cond) \
    do {             \
    } while (0)

#define internal_compiler_error()                                                                                                                     \
    do {                                                                                                                                              \
        printf("Internal compiler error. Either you've found a coimpiler bug, or you've corrupted compiler memory during compile time execution.\n"); \
        exit(1);                                                                                                                                      \
    } while (0)

#endif

void* alloc(u64 size);

char* read_file(const char* path);

f64 get_time_in_seconds();
