#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#define massert(cond, msg)                                                   \
    do {                                                                     \
        if (!(cond)) {                                                       \
            printf("massert failed: %.*s\n", (int)(msg).length, (msg).data); \
            debug_break();                                                   \
        }                                                                    \
    } while (0)
#endif

#define mabort(msg)                                                 \
    do {                                                            \
        printf("Cap Error: %.*s\n", (int)(msg).length, (msg).data); \
        abort();                                                    \
    } while (0)

#define ptr_append(ptr, count, capacity, item)             \
    do {                                                   \
        if (count >= capacity) {                           \
            capacity *= 2;                                 \
            if (capacity == 0) capacity = 8;               \
            void* old_data = ptr;                          \
            ptr = cap_alloc(capacity * sizeof(ptr[0]));    \
            memcpy(ptr, old_data, count * sizeof(ptr[0])); \
        }                                                  \
        ptr[count++] = item;                               \
    } while (0)
