#include <direct.h>

#include "inttypes.h"
#include "signal.h"
#include "stdalign.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#ifdef _WIN32
#include "win_dirent.h"
#define popen _popen
#define pclose _pclose
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
#define internal_compiler_error()                                                                                                                            \
    do {                                                                                                                                                     \
        log_output(log_debug);                                                                                                                               \
        printf("\033[31mInternal compiler error. Either you've found a compiler bug, or you've corrupted compiler memory during compile time execution.\n"); \
        debug_break();                                                                                                                                       \
        exit(1);                                                                                                                                             \
    } while (0)
#else
#define assert(cond) \
    do {             \
    } while (0)

#define internal_compiler_error()                                                                                                                            \
    do {                                                                                                                                                     \
        log_output(log_info);                                                                                                                                \
        printf("\033[31mInternal compiler error. Either you've found a compiler bug, or you've corrupted compiler memory during compile time execution.\n"); \
        exit(1);                                                                                                                                             \
    } while (0)

#endif

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define HOST_IS_LITTLE_ENDIAN 1
#else
#define HOST_IS_BIG_ENDIAN 1
#endif
#elif defined(__linux__) || defined(__GLIBC__)
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define HOST_IS_LITTLE_ENDIAN 1
#else
#define HOST_IS_BIG_ENDIAN 1
#endif
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <machine/endian.h>
#include <sys/types.h>
#if defined(__LITTLE_ENDIAN__) || defined(__BYTE_ORDER) && (__BYTE_ORDER == __LITTLE_ENDIAN)
#define HOST_IS_LITTLE_ENDIAN 1
#else
#define HOST_IS_BIG_ENDIAN 1
#endif
#elif defined(_WIN32)
#define HOST_IS_LITTLE_ENDIAN 1
#else
#error "Endianness unknown: add a configure-time check or platform-specific branch"
#endif

void* alloc(u64 size);

char* read_file(const char* path);

f64 get_time_in_seconds();

bool bit_get(const void* data, u32 bit_index);
void bit_set(void* data, u32 bit_index, bool value);
