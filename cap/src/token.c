#include "cap.h"

Token tokens_get(Tokens tokens) {
    if (tokens.count == 0) return (Token){0};
    return tokens_index(tokens, 0);
}

Token tokens_index(Tokens tokens, u64 index) {
    return tokens.data[index];
}

void tokens_back(Tokens* tokens) {
    tokens->count++;
    tokens->data--;
}

void tokens_next(Tokens* tokens) {
    if (tokens->count == 0) return;
    tokens->count--;
    tokens->data++;
}

bool token_newline_after_token_kind_causes_end_statement(Token_Kind kind) {
    switch (kind) {
        case Token_Kind_Scope_Start:
        case Token_Kind_Assign:
        case Token_Kind_End_Statement:
        case Token_Kind_Comma:
        case Token_Kind_Paren_Open:
            return false;
        case Token_Kind_Paren_Close:
        case Token_Kind_Build:
        case Token_Kind_Identifier:
        case Token_Kind_Return:
        case Token_Kind_Int:
        case Token_Kind_Float:
        case Token_Kind_Scope_End:
        case Token_Kind_End_File:
        case Token_Kind_Invalid:
            return true;
    }
}

utf8 token_kind_to_string(Token_Kind kind) {
    switch (kind) {
        case Token_Kind_Invalid:
            return utf8_str("Token_Kind_Invalid");
        case Token_Kind_Identifier:
            return utf8_str("Token_Kind_Identifier");
        case Token_Kind_End_Statement:
            return utf8_str("Token_Kind_End_Statement");
        case Token_Kind_End_File:
            return utf8_str("Token_Kind_End_File");
        case Token_Kind_Return:
            return utf8_str("Token_Kind_Return");
        case Token_Kind_Int:
            return utf8_str("Token_Kind_Int");
        case Token_Kind_Float:
            return utf8_str("Token_Kind_Float");
        case Token_Kind_Scope_Start:
            return utf8_str("Token_Kind_Scope_Start");
        case Token_Kind_Scope_End:
            return utf8_str("Token_Kind_Scope_End");
        case Token_Kind_Assign:
            return utf8_str("Token_Kind_Assign");
        case Token_Kind_Comma:
            return utf8_str("Token_Kind_Comma");
        case Token_Kind_Paren_Open:
            return utf8_str("Token_Kind_Paren_Open");
        case Token_Kind_Paren_Close:
            return utf8_str("Token_Kind_Paren_Close");
        case Token_Kind_Build:
            return utf8_str("Token_Kind_Build");
    }
}

Token_Kind token_kind_for_identifier(utf8 word) {
    if (utf8_equal(word, utf8_str("return"))) return Token_Kind_Return;
    if (utf8_equal(word, utf8_str("build"))) return Token_Kind_Build;
    return Token_Kind_Identifier;
}

Tokens tokens_from_file(Cap_File* file) {
    Tokens tokens = {0};
    u64 tokens_capacity = 0;

    utf8 remaining_file_contents = file->contents;
    while (remaining_file_contents.count > 0) {
        Token token = {0};
        char* token_start = remaining_file_contents.data;

        u32 unicode = utf8_get(remaining_file_contents);
        utf8_next(&remaining_file_contents);

        // blank space
        if (unicode == ' ' || unicode == '\t' || unicode == '\r') {
            continue;
        }

        // newline
        else if (unicode == '\n') {
            if (tokens.count > 0) {
                Token last_token = tokens.data[tokens.count - 1];
                if (token_newline_after_token_kind_causes_end_statement(last_token.kind)) {
                    token.kind = Token_Kind_End_Statement;
                } else continue;
            } else continue;
        }

        // line extension
        else if (unicode == '\\') {
            unicode = utf8_get(remaining_file_contents);
            while (unicode == ' ' || unicode == '\t' || unicode == '\r') {
                utf8_next(&remaining_file_contents);
                unicode = utf8_get(remaining_file_contents);
            }
            if (unicode == '\n') {
                continue;
            } else {
                u32 word_length = remaining_file_contents.data - token_start;
                utf8 word = {token_start, word_length};
                log_msg_context("Line extension can only be followed by a newline", log_error, word, file);
            }
        }

        // single line comment
        else if (unicode == '/' && utf8_get(remaining_file_contents) == '/') {
            unicode = utf8_get(remaining_file_contents);
            while (unicode != '\n') {
                utf8_next(&remaining_file_contents);
                unicode = utf8_get(remaining_file_contents);
            }
            continue;
        }

        // multi line comment
        else if (unicode == '/' && utf8_get(remaining_file_contents) == '*') {
            while (true) {
                unicode = utf8_get(remaining_file_contents);
                if (unicode == '*' && utf8_get(remaining_file_contents) == '/') {
                    unicode = utf8_get(remaining_file_contents);
                    utf8_next(&remaining_file_contents);
                    break;
                } else if (unicode == 0) {
                    u32 word_length = remaining_file_contents.data - token_start;
                    utf8 word = {token_start, word_length};
                    log_msg_context("Multi line comment did not end", log_error, word, file);
                    break;
                }
                utf8_next(&remaining_file_contents);
            }
            continue;
        }

        // identifier and keyword
        else if (unicode >= 0x80 || isalpha(unicode) || unicode == '_') {
            unicode = utf8_get(remaining_file_contents);
            while (unicode >= 0x80 || isalpha(unicode) || unicode == '_' || (unicode >= '0' && unicode <= '9')) {
                utf8_next(&remaining_file_contents);
                unicode = utf8_get(remaining_file_contents);
            }
            u32 word_length = remaining_file_contents.data - token_start;
            utf8 word = {token_start, word_length};
            token.kind = token_kind_for_identifier(word);
        }

        // number
        else if ((unicode >= '0' && unicode <= '9') || unicode == '.') {
            unicode = utf8_get(remaining_file_contents);
            while (unicode >= '0' && unicode <= '9') {
                utf8_next(&remaining_file_contents);
                unicode = utf8_get(remaining_file_contents);
            }
            if (unicode == '.') {
                utf8_next(&remaining_file_contents);
                unicode = utf8_get(remaining_file_contents);
                while (unicode >= '0' && unicode <= '9') {
                    utf8_next(&remaining_file_contents);
                    unicode = utf8_get(remaining_file_contents);
                }
                token.kind = Token_Kind_Float;
            } else {
                token.kind = Token_Kind_Int;
            }
        }

        // string
        else if (unicode == '"') {
            // TODO: parse string and handel formatting
            assert(false);
        }

        // symbols
        else {
            switch (unicode) {
                case '{': {
                    token.kind = Token_Kind_Scope_Start;
                    break;
                }
                case '}': {
                    token.kind = Token_Kind_Scope_End;
                    break;
                }
                case '=': {
                    token.kind = Token_Kind_Assign;
                    break;
                }
                case ';': {
                    token.kind = Token_Kind_End_Statement;
                    break;
                }
                case ',': {
                    token.kind = Token_Kind_Comma;
                    break;
                }
                case '(': {
                    token.kind = Token_Kind_Paren_Open;
                    break;
                }
                case ')': {
                    token.kind = Token_Kind_Paren_Close;
                    break;
                }
                default: {
                    token.kind = Token_Kind_Invalid;
                    break;
                }
            }
        }

        char* token_end = remaining_file_contents.data;
        u32 token_length = token_end - token_start;
        utf8 token_contents = {token_start, token_length};
        token.data = token_contents;

        utf8 token_type_string = token_kind_to_string(token.kind);

        if (token.kind == Token_Kind_Invalid) {
            log_msg_token("Invalid token", log_error, token, file);
        } else {
            ptr_append(tokens.data, tokens.count, tokens_capacity, token);
            log_msg_token("Token added", log_debug, token, file);
        }
    }

    Token token_end_file = {0};
    token_end_file.kind = Token_Kind_End_File;
    token_end_file.data = remaining_file_contents;
    token_end_file.data.data--;
    token_end_file.data.count++;
    utf8 token_end_file_type_string = token_kind_to_string(token_end_file.kind);
    log_msg_token("End file token added", log_debug, token_end_file, file);
    ptr_append(tokens.data, tokens.count, tokens_capacity, token_end_file);

    return tokens;
}
