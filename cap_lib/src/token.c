#include "cap/token.h"

#include "cap.h"
#include "cap/log.h"
#include "cap/string.h"

bool token_is_assign(Token_Kind kind) {
    switch (kind) {
        case token_assign:
        case token_add_assign:
        case token_subtract_assign:
        case token_multiply_assign:
        case token_divide_assign:
        case token_modulo_assign:
        case token_shift_left_assign:
        case token_shift_right_assign:
        case token_bitwise_and_assign:
        case token_bitwise_or_assign:
        case token_bitwise_xor_assign:
            return true;
        default:
            return false;
    }
}

bool token_is_assign_and_operator(Token_Kind kind) {
    switch (kind) {
        case token_add_assign:
        case token_subtract_assign:
        case token_multiply_assign:
        case token_divide_assign:
        case token_modulo_assign:
        case token_shift_left_assign:
        case token_shift_right_assign:
        case token_bitwise_and_assign:
        case token_bitwise_or_assign:
        case token_bitwise_xor_assign:
            return true;
        default:
            return false;
    }
}

bool _token_should_insert_endstatement(Token* token) {
    switch (token->kind) {
        case token_identifier:
        case token_int:
        case token_float:
        case token_return:
        case token_if:
        case token_program:
        case token_end_scope:
        case token_not:
        case token_multiply:
        case token_bitwise_and:
        case token_paren_close:
        case token_string_block_end:
        case token_as:
        case token_include:
            return true;
        case token_hashtag:
        case token_string_block:
        case token_bitwise_xor:
        case token_shift_left_assign:
        case token_shift_right_assign:
        case token_bitwise_and_assign:
        case token_bitwise_or_assign:
        case token_bitwise_xor_assign:
        case token_divide:
        case token_modulo:
        case token_subtract:
        case token_add:
        case token_shift_left:
        case token_shift_right:
        case token_less:
        case token_greater:
        case token_less_equal:
        case token_greater_equal:
        case token_equal:
        case token_not_equal:
        case token_bitwise_or:
        case token_logical_and:
        case token_logical_or:
        case token_assign:
        case token_arrow:
        case token_add_assign:
        case token_subtract_assign:
        case token_multiply_assign:
        case token_divide_assign:
        case token_modulo_assign:
        case token_end_file:
        case token_begin_scope:
        case token_invalid:
        case token_end_statement:
        case token_comma:
        case token_paren_open:
        case token_colon_colon:
        case token_colon:
            return false;
    }
}

String token_tokens_to_string(Tokens tokens) {
    String str = {0};
    for (u64 i = 0; i < tokens.count; i++) {
        Token* token = &tokens.data[i];
        String token_str = token_token_to_string(*token);
        str = string_append(str, str(" "));
        str = string_append(str, token_str);
        if (token->kind == token_end_statement) str = string_append(str, str("\n"));
    }
    return str;
}

String token_token_kind_to_string(Token_Kind kind) {
    switch (kind) {
        case token_invalid:
            return str("invalid");
        case token_string_block:
            return str("string_block");
        case token_comma:
            return str("comma");
        case token_identifier:
            return str("identifier");
        case token_program:
            return str("program");
        case token_return:
            return str("return");
        case token_if:
            return str("if");
        case token_end_statement:
            return str("end_statement");
        case token_end_file:
            return str("end_file");
        case token_int:
            return str("int");
        case token_float:
            return str("float");
        case token_begin_scope:
            return str("begin_scope");
        case token_end_scope:
            return str("end_scope");
        case token_add:
            return str("add");
        case token_subtract:
            return str("subtract");
        case token_multiply:
            return str("multiply");
        case token_divide:
            return str("divide");
        case token_modulo:
            return str("modulo");
        case token_greater:
            return str("greater");
        case token_less:
            return str("less");
        case token_greater_equal:
            return str("greater_equal");
        case token_less_equal:
            return str("less_equal");
        case token_logical_and:
            return str("logical_and");
        case token_bitwise_and:
            return str("bitwise_and");
        case token_logical_or:
            return str("logical_or");
        case token_bitwise_or:
            return str("bitwise_or");
        case token_bitwise_xor:
            return str("bitwise_xor");
        case token_shift_left:
            return str("shift_left");
        case token_shift_right:
            return str("shift_right");
        case token_assign:
            return str("assign");
        case token_arrow:
            return str("arrow");
        case token_equal:
            return str("equal");
        case token_not_equal:
            return str("not_equal");
        case token_not:
            return str("not");
        case token_add_assign:
            return str("add_assign");
        case token_subtract_assign:
            return str("subtract_assign");
        case token_multiply_assign:
            return str("multiply_assign");
        case token_divide_assign:
            return str("divide_assign");
        case token_modulo_assign:
            return str("modulo_assign");
        case token_shift_left_assign:
            return str("shift_left_assign");
        case token_shift_right_assign:
            return str("shift_right_assign");
        case token_bitwise_and_assign:
            return str("bitwise_and_assign");
        case token_bitwise_or_assign:
            return str("bitwise_or_assign");
        case token_bitwise_xor_assign:
            return str("bitwise_xor_assign");
        case token_paren_open:
            return str("paren_open");
        case token_paren_close:
            return str("paren_close");
        case token_hashtag:
            return str("hashtag");
        case token_include:
            return str("include");
        case token_string_block_end:
            return str("string_block_end");
        case token_as:
            return str("as");
        case token_colon_colon:
            return str("colon_colon");
        case token_colon:
            return str("colon");
    }
}

u64 token_precedence(Token_Kind kind) {
    switch (kind) {
        case token_multiply:
        case token_divide:
        case token_modulo:
            return 1;
        case token_subtract:
        case token_add:
            return 2;
        case token_shift_left:
        case token_shift_right:
            return 3;
        case token_less:
        case token_greater:
        case token_less_equal:
        case token_greater_equal:
            return 4;
        case token_equal:
        case token_not_equal:
            return 5;
        case token_bitwise_and:
            return 6;
        case token_bitwise_xor:
            return 7;
        case token_bitwise_or:
            return 8;
        case token_logical_and:
            return 9;
        case token_logical_or:
            return 10;
        case token_as:
        case token_paren_open:
        case token_paren_close:
        case token_string_block:
        case token_comma:
        case token_not:
        case token_assign:
        case token_arrow:
        case token_string_block_end:
        case token_add_assign:
        case token_subtract_assign:
        case token_multiply_assign:
        case token_divide_assign:
        case token_modulo_assign:
        case token_int:
        case token_float:
        case token_identifier:
        case token_return:
        case token_if:
        case token_program:
        case token_end_scope:
        case token_begin_scope:
        case token_invalid:
        case token_end_file:
        case token_end_statement:
        case token_shift_left_assign:
        case token_shift_right_assign:
        case token_bitwise_and_assign:
        case token_bitwise_or_assign:
        case token_bitwise_xor_assign:
        case token_hashtag:
        case token_include:
        case token_colon_colon:
        case token_colon:
            return UINT64_MAX;
    }
}

i64 token_int_value(Token token) {
    massert(token.kind == token_int, str("Token is not an int"));
    String content = token.content;
    char buffer[1024];
    for (u64 i = 0; i < content.length; i++) {
        char c = content.data[i];
        if (c == '_') continue;
        buffer[i] = c;
    }
    buffer[content.length] = '\0';
    return strtol(buffer, NULL, 10);
}

f64 token_float_value(Token token) {
    massert(token.kind == token_float, str("Token is not a float"));
    String content = token.content;
    char buffer[1024];
    for (u64 i = 0; i < content.length; i++) {
        char c = content.data[i];
        if (c == '_') continue;
        buffer[i] = c;
    }
    buffer[content.length] = '\0';
    return strtod(buffer, NULL);
}

String token_token_to_string(Token token) {
    String content = token.content;
    if (content.length != 0 && content.data[content.length - 1] == '\n') {
        content.length -= 1;
        content = string_append(content, str("\\n"));
    }

    String kind_name = token_token_kind_to_string(token.kind);
    char buffer[1024];
    snprintf(buffer, 1024, "%.*s(%.*s)", str_info(kind_name), str_info(content));

    u64 length = strlen(buffer);
    char* ptr = cap_alloc(length + 1);
    memcpy(ptr, buffer, length);

    return string_create(ptr, length);
}

void token_next(Tokens tokens, u64* i) {
    *i += 1;
}

Token token_get(Tokens tokens, u64* i) {
    return tokens.data[*i];
}

void token_set(Tokens tokens, u64* i, u64 new_i) {
    *i = new_i;
}

void token_previous(Tokens tokens, u64* i) {
    *i -= 1;
}

static bool _token_is_multiline_string(char* string_start) {
    if (string_start[1] == '\"' && string_start[2] == '\"') return true;
    return false;
}

bool token_string_block_is_start_of_string(Token token) {
    char c = token.content.data[0];
    return c == '\"';
}

String token_get_string_block_str(Token token) {
    massert(token.kind == token_string_block || token.kind == token_string_block_end, str("Expected string block"));
    bool is_multiline = _token_is_multiline_string(token.content.data);
    bool is_end_of_string = token.kind == token_string_block_end;
    bool is_start_of_string = token_string_block_is_start_of_string(token);
    String substr = {0};
    if (is_multiline) {
        if (is_start_of_string) {
            substr.data = token.content.data + 3;
            if (is_end_of_string) {
                substr.length = token.content.length - 6;
            } else {
                substr.length = token.content.length - 4;
            }
        } else {
            substr.data = token.content.data + 1;
            if (is_end_of_string) {
                substr.length = token.content.length - 4;
            } else {
                substr.length = token.content.length - 2;
            }
        }
    } else {
        substr.data = token.content.data + 1;
        substr.length = token.content.length - 2;
    }

    String escape_sequence_fixed;
    escape_sequence_fixed.data = cap_alloc(substr.length * sizeof(char));
    escape_sequence_fixed.length = 0;
    for (u64 i = 0; i < substr.length; i++) {
        char c = substr.data[i];
        if (c == '\\') {
            i++;
            c = substr.data[i];
            switch (c) {
                case 'n': {
                    escape_sequence_fixed.data[escape_sequence_fixed.length++] = '\n';
                    break;
                }
                case 'r': {
                    escape_sequence_fixed.data[escape_sequence_fixed.length++] = '\r';
                    break;
                }
                case 't': {
                    escape_sequence_fixed.data[escape_sequence_fixed.length++] = '\t';
                    break;
                }
                case '0': {
                    escape_sequence_fixed.data[escape_sequence_fixed.length++] = '\0';
                    break;
                }
                case '\\': {
                    escape_sequence_fixed.data[escape_sequence_fixed.length++] = '\\';
                    break;
                }
                case '"': {
                    escape_sequence_fixed.data[escape_sequence_fixed.length++] = '"';
                    break;
                }
                case '\'': {
                    escape_sequence_fixed.data[escape_sequence_fixed.length++] = '\'';
                    break;
                }
                default: {
                    String escape_substr;
                    escape_substr.data = substr.data + i - 1;
                    escape_substr.length = 2;
                    log_error_substring(escape_substr, "Invalid escape sequence");
                    escape_sequence_fixed.data[escape_sequence_fixed.length++] = c;
                    break;
                }
            }
        } else {
            escape_sequence_fixed.data[escape_sequence_fixed.length++] = c;
        }
    }
    return escape_sequence_fixed;
}

Token_Kind _token_get_keyword_kind(String keyword) {
    if (string_equal(keyword, str("program"))) return token_program;
    if (string_equal(keyword, str("return"))) return token_return;
    if (string_equal(keyword, str("if"))) return token_if;
    if (string_equal(keyword, str("include"))) return token_include;
    if (string_equal(keyword, str("as"))) return token_as;
    return token_identifier;
}

Tokens token_tokenize(Cap_File* file) {
    String file_content = file->content;
    char* walk = file_content.data;
    Tokens tokens = {0};
    u64 capacity = 8;
    tokens.data = cap_alloc(capacity * sizeof(tokens.data[0]));
    tokens.count = 0;

    u64 string_stack_count = 0;
    char* string_stack[1024];

    while (walk < file_content.data + file_content.length) {
        Token token = {0};
        String content = {0};
        content.data = walk;

        char c = *walk;
        if (c == ' ' || c == '\t' || c == '\r') {
            walk++;
            continue;
        } else if (c == '\n') {
            walk++;
            Token* last_token = tokens.data + tokens.count - 1;
            if (!_token_should_insert_endstatement(last_token)) continue;
            token.kind = token_end_statement;
        } else if (c == ';') {
            walk++;
            token.kind = token_end_statement;
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
                c = *++walk;
            }
            content.length = walk - content.data;
            token.kind = _token_get_keyword_kind(content);
        } else if ((c >= '0' && c <= '9') || c == '.') {
            bool is_float = false;
            while ((c >= '0' && c <= '9') || c == '.' || c == '_') {
                if (c == '.') {
                    if (is_float) break;
                    is_float = true;
                }
                c = *++walk;
            }
            if (is_float) {
                token.kind = token_float;
            } else {
                token.kind = token_int;
            }
        } else if (c == '{') {
            walk++;
            token.kind = token_begin_scope;
        } else if (c == '}') {
            walk++;
            if (string_stack_count == 0) {
                token.kind = token_end_scope;
            } else {
                token.kind = token_string_block;
                char* string_start = string_stack[string_stack_count - 1];
                bool is_multiline = _token_is_multiline_string(string_start);
                while (true) {
                    char c = *walk;
                    if (c == '\n' && !is_multiline) {
                        String error_substr;
                        error_substr.data = string_start;
                        error_substr.length = is_multiline ? 3 : 1;
                        log_error_substring(error_substr, "Unterminated string");
                        token.kind = token_invalid;
                        break;
                    }
                    if (c == '"') {
                        walk++;
                        if (is_multiline) {
                            if (walk[0] != '\"' || walk[1] != '\"') {
                                String error_substr;
                                error_substr.data = walk;
                                error_substr.length = is_multiline ? 3 : 1;
                                log_error_substring(error_substr, "Must terminate string with \"\"\"");
                                token.kind = token_invalid;
                                break;
                            }
                            walk += 2;
                        }
                        string_stack_count--;
                        token.kind = token_string_block_end;
                        break;
                    }
                    if (c == '\0') {
                        String error_substr;
                        error_substr.data = string_start;
                        error_substr.length = is_multiline ? 3 : 1;
                        log_error_substring(error_substr, "Unterminated string");
                        token.kind = token_invalid;
                        break;
                    }
                    if (c == '{') {
                        walk++;
                        break;
                    }
                    walk++;
                }
            }

        } else if (c == '+') {
            walk++;
            c = *walk;
            if (c == '=') {
                walk++;
                token.kind = token_add_assign;
            } else {
                token.kind = token_add;
            }
        } else if (c == '-') {
            walk++;
            c = *walk;
            if (c == '=') {
                walk++;
                token.kind = token_subtract_assign;
            } else {
                token.kind = token_subtract;
            }
        } else if (c == '*') {
            walk++;
            c = *walk;
            if (c == '=') {
                walk++;
                token.kind = token_multiply_assign;
            } else {
                token.kind = token_multiply;
            }
        } else if (c == '%') {
            walk++;
            c = *walk;
            if (c == '=') {
                walk++;
                token.kind = token_modulo_assign;
            } else {
                token.kind = token_modulo;
            }
        } else if (c == '/') {
            walk++;
            c = *walk;
            if (c == '=') {
                walk++;
                token.kind = token_divide_assign;
            } else {
                token.kind = token_divide;
            }
        } else if (c == '>') {
            walk++;
            c = *walk;
            if (c == '=') {
                walk++;
                token.kind = token_greater_equal;
            } else if (c == '>') {
                walk++;
                c = *walk;
                if (c == '=') {
                    walk++;
                    token.kind = token_shift_right_assign;
                } else {
                    token.kind = token_shift_right;
                }
            } else {
                token.kind = token_greater;
            }
        } else if (c == '<') {
            walk++;
            c = *walk;
            if (c == '=') {
                walk++;
                token.kind = token_less_equal;
            } else if (c == '<') {
                walk++;
                c = *walk;
                if (c == '=') {
                    walk++;
                    token.kind = token_shift_left_assign;
                } else {
                    token.kind = token_shift_left;
                }
            } else {
                token.kind = token_less;
            }
        } else if (c == '&') {
            walk++;
            c = *walk;
            if (c == '&') {
                walk++;
                token.kind = token_logical_and;
            } else if (c == '=') {
                walk++;
                token.kind = token_bitwise_and_assign;
            } else {
                token.kind = token_bitwise_and;
            }
        } else if (c == '|') {
            walk++;
            c = *walk;
            if (c == '|') {
                walk++;
                token.kind = token_logical_or;
            } else if (c == '=') {
                walk++;
                token.kind = token_bitwise_or_assign;
            } else {
                token.kind = token_bitwise_or;
            }
        } else if (c == '^') {
            walk++;
            c = *walk;
            if (c == '=') {
                walk++;
                token.kind = token_bitwise_xor_assign;
            } else {
                token.kind = token_bitwise_xor;
            }
        } else if (c == '=') {
            walk++;
            c = *walk;
            if (c == '=') {
                walk++;
                token.kind = token_equal;
            } else if (c == '>') {
                walk++;
                token.kind = token_arrow;
            } else {
                token.kind = token_assign;
            }
        } else if (c == '!') {
            walk++;
            c = *walk;
            if (c == '=') {
                walk++;
                token.kind = token_not_equal;
            } else {
                token.kind = token_not;
            }
        } else if (c == ',') {
            walk++;
            token.kind = token_comma;
        } else if (c == '(') {
            walk++;
            token.kind = token_paren_open;
        } else if (c == ')') {
            walk++;
            token.kind = token_paren_close;
        } else if (c == '#') {
            walk++;
            token.kind = token_hashtag;
        } else if (c == ':') {
            walk++;
            if (*walk == ':') {
                walk++;
                token.kind = token_colon_colon;
            } else {
                token.kind = token_colon;
            }
        } else if (c == '"') {
            walk++;
            token.kind = token_string_block;
            char* string_start = walk;
            string_stack[string_stack_count++] = string_start;
            bool is_multiline = _token_is_multiline_string(string_start);
            if (is_multiline) walk += 2;
            while (true) {
                char c = *walk;
                if (c == '\n' && !is_multiline) {
                    String error_substr;
                    error_substr.data = string_start;
                    error_substr.length = is_multiline ? 3 : 1;
                    log_error_substring(error_substr, "Unterminated string");
                    break;
                }
                if (c == '"') {
                    walk++;
                    if (is_multiline) {
                        if (walk[0] != '\"' || walk[1] != '\"') {
                            String error_substr;
                            error_substr.data = walk;
                            error_substr.length = is_multiline ? 3 : 1;
                            log_error_substring(error_substr, "Must terminate string with \"\"\"");
                            break;
                        }
                        walk += 2;
                    }
                    string_stack_count--;
                    token.kind = token_string_block_end;
                    break;
                }
                if (c == '\0') {
                    String error_substr;
                    error_substr.data = string_start;
                    error_substr.length = is_multiline ? 3 : 1;
                    log_error_substring(error_substr, "Unterminated string");
                    break;
                }
                if (c == '{') {
                    walk++;
                    break;
                }
                walk++;
            }
        } else {
            walk++;
            token.kind = token_invalid;
            content.length = walk - content.data;
            token.content = content;
            log_error_token(file, token, "Invalid token");
        }

        content.length = walk - content.data;
        token.content = content;
        ptr_append(tokens.data, tokens.count, capacity, token);
    }
    Token end_file_token = {0};
    end_file_token.kind = token_end_file;
    end_file_token.content = string_create(walk, 0);
    ptr_append(tokens.data, tokens.count, capacity, end_file_token);
    return tokens;
}
