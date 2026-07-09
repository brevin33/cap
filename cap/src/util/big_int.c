#include "big_int.h"

Big_Int big_from_int(i64 data) {
    Big_Int res = {0};
    res.data = data;
    return res;
}

Big_Int big_from_utf8(utf8 data, bool* err) {
    Big_Int res = {0};
    char* str = utf8_to_str(data);
    res.data = atoll(str);
    if (res.data == LLONG_MAX && errno != 0) {
        *err = true;
        return res;
    }
    return res;
}

Big_Int big_from_str(const char* data, bool* err) {
    Big_Int res = {0};
    res.data = atoll(data);
    if (res.data == LLONG_MAX && errno != 0) {
        *err = true;
        return res;
    }
    return res;
}

Big_Int big_add(Big_Int a, Big_Int b) {
    Big_Int res;
    res.data = a.data + b.data;
    return res;
}

Big_Int big_sub(Big_Int a, Big_Int b) {
    Big_Int res;
    res.data = a.data - b.data;
    return res;
}

Big_Int big_mul(Big_Int a, Big_Int b) {
    Big_Int res;
    res.data = a.data * b.data;
    return res;
}

Big_Int big_div(Big_Int a, Big_Int b) {
    Big_Int res;
    res.data = a.data / b.data;
    return res;
}

Big_Int big_mod(Big_Int a, Big_Int b) {
    Big_Int res;
    res.data = a.data % b.data;
    return res;
}

bool big_equal(Big_Int a, Big_Int b) {
    return a.data == b.data;
}

bool big_less(Big_Int a, Big_Int b) {
    return a.data < b.data;
}

bool big_greater(Big_Int a, Big_Int b) {
    return a.data > b.data;
}

bool big_less_equal(Big_Int a, Big_Int b) {
    return a.data <= b.data;
}

bool big_greater_equal(Big_Int a, Big_Int b) {
    return a.data >= b.data;
}

utf8 big_to_utf8(Big_Int a) {
    char* str = big_to_str(a);
    utf8 res = utf8_str(str);
    return res;
}

char* big_to_str(Big_Int a) {
    char buffer[4096];
    sprintf(buffer, "%lld", a.data);
    char* res = alloc(strlen(buffer) + 1);
    memcpy(res, buffer, strlen(buffer));
    res[strlen(buffer)] = 0;
    return res;
}

i64 big_to_i64(Big_Int a, bool* err) {
    return a.data;
}

u64 big_to_u64(Big_Int a, bool* err) {
    return a.data;
}
