#pragma once

#define _CRT_SECURE_NO_WARNINGS

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

#define arr_len(arr) (sizeof(arr) / sizeof(arr[0]))

#ifdef NDEBUG
#define massert(cond, msg)
#else
#define massert(cond, msg)                                             \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("massert failed: %*s\n", (msg).length, (msg).data); \
            exit(1);                                                   \
        }                                                              \
    } while (0)
#endif

#define mabort(msg)                                                  \
    do {                                                             \
        printf("Cap Error: : %*s\n", (int)(msg).length, (msg).data); \
        exit(1);                                                     \
    } while (0)

void* cap_alloc(u64 size);

void cap_init();

void cap_print(char* format, ...);
