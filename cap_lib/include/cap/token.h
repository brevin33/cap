#pragma once
#include "cap/base.h"
#include "cap/string.h"

typedef struct Tokens Tokens;
typedef struct Token Token;

typedef enum Token_Kind {
    token_invalid = 0,
} Token_Kind;

struct Token {
    Token_Kind kind;
    String content;
};

struct Tokens {
    String file_content;
    Token* data;
    u64 count;
};

Tokens token_tokenize(String file_content);
