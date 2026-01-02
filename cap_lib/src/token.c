#include "cap.h"
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
            return true;
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
        case token_paren_open:
        case token_paren_close:
        case token_comma:
        case token_not:
        case token_assign:
        case token_arrow:
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

Tokens token_tokenize(Cap_File* file) {
    String file_content = file->content;
    char* walk = file_content.data;
    Tokens tokens = {0};
    u64 capacity = 8;
    tokens.data = cap_alloc(capacity * sizeof(tokens.data[0]));
    tokens.count = 0;
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
            if (string_equal(content, str("program"))) {
                token.kind = token_program;
            } else if (string_equal(content, str("return"))) {
                token.kind = token_return;
            } else if (string_equal(content, str("if"))) {
                token.kind = token_if;
            } else {
                token.kind = token_identifier;
            }
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
            token.kind = token_end_scope;
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
