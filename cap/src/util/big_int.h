#pragma once

#include "base.h"
#include "utf8.h"
// this is not a real big int just a placeholder I can switchout later

typedef struct Big_Int Big_Int;

struct Big_Int {
    i64 data;
};

Big_Int big_from_int(i64 data);

Big_Int big_from_utf8(utf8 data, bool* err);

Big_Int big_from_str(const char* data, bool* err);

Big_Int big_add(Big_Int a, Big_Int b);

Big_Int big_sub(Big_Int a, Big_Int b);

Big_Int big_mul(Big_Int a, Big_Int b);

Big_Int big_div(Big_Int a, Big_Int b);

Big_Int big_mod(Big_Int a, Big_Int b);

bool big_equal(Big_Int a, Big_Int b);

bool big_less(Big_Int a, Big_Int b);

bool big_greater(Big_Int a, Big_Int b);

bool big_less_equal(Big_Int a, Big_Int b);

bool big_greater_equal(Big_Int a, Big_Int b);

utf8 big_to_utf8(Big_Int a);

char* big_to_str(Big_Int a);

i64 big_to_i64(Big_Int a, bool* err);

u64 big_to_u64(Big_Int a, bool* err);
