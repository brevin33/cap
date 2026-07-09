#pragma once

#include "util/utf8.h"
typedef struct Token Token;
typedef struct Tokens Tokens;
typedef struct Cap_File Cap_File;

typedef enum Token_Kind {
    Token_Kind_Invalid = 0,
    Token_Kind_Identifier,
    Token_Kind_End_Statement,
    Token_Kind_End_File,
    Token_Kind_Return,
    Token_Kind_Build,
    Token_Kind_Int,
    Token_Kind_Float,
    Token_Kind_Scope_Start,
    Token_Kind_Scope_End,
    Token_Kind_Paren_Open,
    Token_Kind_Paren_Close,
    Token_Kind_Assign,
    Token_Kind_Comma,
} Token_Kind;

struct Token {
    Token_Kind kind;
    utf8 data;
};

struct Tokens {
    Token* data;
    u64 count;
};

Token tokens_get(Tokens tokens);

Token tokens_index(Tokens tokens, u64 index);

void tokens_back(Tokens* tokens);

void tokens_next(Tokens* tokens);

Tokens tokens_from_file(Cap_File* data);

Token_Kind token_kind_for_identifier(utf8 word);

utf8 token_kind_to_string(Token_Kind kind);

bool token_newline_after_token_kind_causes_end_statement(Token_Kind kind);
