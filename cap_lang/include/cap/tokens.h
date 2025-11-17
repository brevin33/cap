#pragma once
#include "base.h"
typedef struct File File;

typedef enum TokenType {
    tt_invalid = 0,
    tt_id,
    tt_int,
    tt_float,
    tt_string,
    tt_end_statement,
    tt_eof,
    tt_lbrace,
    tt_rbrace,
    tt_lparen,
    tt_rparen,
    tt_lbracket,
    tt_rbracket,
    tt_comma,
    tt_dot,
    tt_equals,
    // keywords
    tt_return,
    tt_program,
} TokenType;

typedef struct Token {
    TokenType type;
    u32 start_char;
    u32 end_char;
    u32 file_index;
} Token;

Token *tokenize(u32 file_index);

bool token_last_ends_statement_and_endline(Token *token);

void token_print(Token *token);

char *token_get_string(Token *token);

char *token_get_id(Token *token);

u64 token_get_int(Token *token);

double token_get_float(Token *token);
