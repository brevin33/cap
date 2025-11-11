#include "base/basic.h"
#include "cap.h"

void token_swap_for_keyword(Token *token) {
    massert(token->type == tt_id, "token type is not tt_id");
    char *id = token_get_id(token);
    if (strcmp(id, "return") == 0) {
        token->type = tt_return;
    }
}

bool token_is_number_char(char c, bool *is_float) {
    switch (c) {
        case '.': {
            if (*is_float) return false;
            *is_float = true;
            return true;
        }
        CASE_NUMBER: {
            return true;
        }
        default: {
            return false;
        }
    }
}

bool token_is_id_char(char c) {
    switch (c) {
        case '_':
        CASE_NUMBER:
        CASE_LETTER: {
            return true;
        }
        default: {
            return false;
        }
    }
}

bool token_last_ends_statement_and_endline(Token *token) {
    switch (token->type) {
        case tt_string:
        case tt_int:
        case tt_float:
        case tt_id:
        case tt_rbrace:
        case tt_rparen:
        case tt_rbracket:
        case tt_return:
            return true;
        case tt_invalid:
        case tt_equals:
        case tt_comma:
        case tt_dot:
        case tt_lbracket:
        case tt_lbrace:
        case tt_lparen:
        case tt_end_statement:
        case tt_eof:
            return false;
    }
}

Token *tokenize(u32 file_index) {
    File *file = *File_Ptr_List_get(&cap_context.files, file_index);
    char *input = file->contents;

    u32 i = 0;
    Token last_token = {tt_invalid, 0, 0, file_index};
    u32 tokens_capacity = 512;
    u32 tokens_count = 0;
    Token *tokens = alloc(sizeof(Token) * tokens_capacity);

    while (input[i] != '\0') {
        TokenType type = tt_invalid;
        u32 start_char = i;
        switch (input[i]) {
            case ' ':
            case '\t':
            case '\r': {
                i++;
                continue;
            }
            case '\n': {
                if (token_last_ends_statement_and_endline(&last_token)) {
                    type = tt_end_statement;
                    i++;
                    break;
                }
                i++;
                continue;
            }
            case '.': {
                bool read_as_number = false;
                switch (input[i + 1]) {
                    default: {
                        type = tt_dot;
                        i++;
                        break;
                    }
                    CASE_NUMBER: {
                        read_as_number = true;
                        break;
                    }
                }
                if (!read_as_number) break;
                [[fallthrough]];
            }
            CASE_NUMBER: {
                bool is_float = false;
                if (input[i] == '.') is_float = true;
                while (token_is_number_char(input[i], &is_float)) i++;
                if (is_float)
                    type = tt_float;
                else
                    type = tt_int;
                break;
            }
            case '_':
            CASE_LETTER: {
                type = tt_id;
                while (token_is_id_char(input[i])) i++;
                break;
            }
            case '"': {
                type = tt_string;
                i++;
                while (input[i] != '"') {
                    if (input[i] == '\0') {
                        log_error(file_index, start_char, 0, "unterminated string");
                        break;
                    }
                    i++;
                }
                break;
            }
            case '{': {
                i++;
                type = tt_lbrace;
                break;
            }
            case '}': {
                i++;
                type = tt_rbrace;
                break;
            }
            case '(': {
                i++;
                type = tt_lparen;
                break;
            }
            case ')': {
                i++;
                type = tt_rparen;
                break;
            }
            case ';': {
                i++;
                type = tt_end_statement;
                break;
            }
            case ',': {
                i++;
                type = tt_comma;
                break;
            }
            case '=': {
                i++;
                type = tt_equals;
                break;
            }
            default: {
                log_error(file_index, start_char, i, "invalid token");
                i++;
                break;
            }
        }
        u32 end_char = i;

        Token token = {type, start_char, end_char, file_index};
        if (token.type == tt_id) token_swap_for_keyword(&token);
        if (tokens_count == tokens_capacity) {
            Token *new_tokens = alloc(sizeof(Token) * tokens_capacity * 2);
            memcpy(new_tokens, tokens, sizeof(Token) * tokens_capacity);
            tokens = new_tokens;
            tokens_capacity *= 2;
        }
        tokens[tokens_count++] = token;
        last_token = token;
    }

    Token end_token = {tt_eof, i, i, file_index};
    if (tokens_count == tokens_capacity) {
        Token *new_tokens = alloc(sizeof(Token) * tokens_capacity * 2);
        memcpy(new_tokens, tokens, sizeof(Token) * tokens_capacity);
        tokens = new_tokens;
        tokens_capacity *= 2;
    }
    tokens[tokens_count++] = end_token;
    return tokens;
}

char *token_get_string(Token *token) {
    File *file = *File_Ptr_List_get(&cap_context.files, token->file_index);
    u32 string_size = token->end_char - token->start_char - 2;
    char *string = alloc(string_size + 1);
    memcpy(string, &file->contents[token->start_char + 1], string_size);
    string[string_size] = '\0';
    return string;
}

char *token_get_id(Token *token) {
    File *file = *File_Ptr_List_get(&cap_context.files, token->file_index);
    u32 id_size = token->end_char - token->start_char;
    char *id = alloc(id_size + 1);
    memcpy(id, &file->contents[token->start_char], id_size);
    id[id_size] = '\0';
    return id;
}

u64 token_get_int(Token *token) {
    bool error = false;
    u64 value = get_string_uint(token_get_id(token), &error);
    if (error) {
        log_error_token(token, "invalid number");
        massert(false, "should never happen");
    }
    return value;
}

double token_get_float(Token *token) {
    bool error = false;
    double value = get_string_float(token_get_id(token), &error);
    if (error) {
        log_error_token(token, "invalid number");
        massert(false, "should never happen");
    }
    return value;
}

void token_print(Token *token) {
    switch (token->type) {
        case tt_lbrace: {
            printf("{ ");
            break;
        }
        case tt_rbrace: {
            printf("} ");
            break;
        }
        case tt_lparen: {
            printf("( ");
            break;
        }
        case tt_rparen: {
            printf(") ");
            break;
        }
        case tt_lbracket: {
            printf("] ");
            break;
        }
        case tt_rbracket: {
            printf("[ ");
            break;
        }
        case tt_comma: {
            printf(", ");
            break;
        }
        case tt_equals: {
            printf("= ");
            break;
        }
        case tt_dot: {
            printf(". ");
            break;
        }
        case tt_return: {
            printf("return ");
            break;
        }
        case tt_id: {
            char *id = token_get_id(token);
            printf("id(%s) ", id);
            break;
        }
        case tt_int: {
            u64 value = token_get_int(token);
            printf("int(%llu) ", value);
            break;
        }
        case tt_float: {
            double value = token_get_float(token);
            printf("float(%f) ", value);
            break;
        }
        case tt_string: {
            char *string = token_get_string(token);
            printf("string(\"%s\") ", string);
            break;
        }
        case tt_end_statement: {
            char *text = token_get_id(token);
            if (strcmp(text, ";") == 0) {
                printf("end_statement(;) ");
            } else {
                printf("end_statement\n");
            }
            break;
        }
        case tt_eof: {
            printf("eof ");
            break;
        }
        case tt_invalid: {
            char *text = token_get_id(token);
            printf("invalid(%s) ", text);
            break;
        }
    }
}
