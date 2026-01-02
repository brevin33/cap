#pragma once
#include "cap/base.h"
#include "cap/string.h"

typedef struct Tokens Tokens;
typedef struct Token Token;
typedef struct Cap_File Cap_File;

typedef enum Token_Kind {
    token_invalid = 0,
    token_identifier,
    token_program,
    token_return,
    token_if,
    token_end_statement,
    token_end_file,
    token_int,
    token_float,
    token_begin_scope,
    token_end_scope,
    token_add,
    token_subtract,
    token_multiply,
    token_divide,
    token_modulo,
    token_greater,
    token_less,
    token_greater_equal,
    token_less_equal,
    token_logical_and,
    token_bitwise_and,
    token_logical_or,
    token_bitwise_xor,
    token_bitwise_or,
    token_shift_left,
    token_shift_right,
    token_assign,
    token_arrow,
    token_equal,
    token_not_equal,
    token_not,
    token_add_assign,
    token_subtract_assign,
    token_multiply_assign,
    token_divide_assign,
    token_modulo_assign,
    token_shift_left_assign,
    token_shift_right_assign,
    token_bitwise_and_assign,
    token_bitwise_or_assign,
    token_bitwise_xor_assign,
    token_comma,
    token_paren_open,
    token_paren_close,
} Token_Kind;

struct Token {
    Token_Kind kind;
    String content;
};

struct Tokens {
    Token* data;
    u64 count;
};

Tokens token_tokenize(Cap_File* file);

String token_tokens_to_string(Tokens tokens);

String token_token_to_string(Token token);

String token_token_kind_to_string(Token_Kind kind);

f64 token_float_value(Token token);

i64 token_int_value(Token token);

u64 token_precedence(Token_Kind kind);

void token_next(Tokens tokens, u64* i);

Token token_get(Tokens tokens, u64* i);

void token_set(Tokens tokens, u64* i, u64 new_i);

void token_previous(Tokens tokens, u64* i);

bool _token_should_insert_endstatement(Token* token);

bool token_is_assign(Token_Kind kind);
bool token_is_assign_and_operator(Token_Kind kind);
