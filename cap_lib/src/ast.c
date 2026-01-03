#include "cap.h"

Ast ast_parse_program(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Ast ast = {0};
    ast.kind = ast_program;
    ast.tokens.data = tokens.data + *i;
    ast.file = file;

    token_next(tokens, &tid);
    Token token = token_get(tokens, &tid);
    ast_expect(token, token_identifier, file);
    ast.program.name = token.content;
    token_next(tokens, &tid);

    ast.program.body = cap_alloc(sizeof(Ast));
    *ast.program.body = ast_function_scope_parse(tokens, &tid, file);
    if (ast.program.body->kind == ast_invalid) return (Ast){0};

    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_function_scope_parse(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Ast ast = {0};
    ast.kind = ast_function_scope;
    ast.tokens.data = tokens.data + *i;
    ast.file = file;

    Token token = token_get(tokens, &tid);
    ast_expect(token, token_begin_scope, file);
    token_next(tokens, &tid);

    u64 statement_capacity = 8;
    ast.function_scope.statements = cap_alloc(statement_capacity * sizeof(ast.function_scope.statements[0]));
    ast.function_scope.count = 0;

    token = token_get(tokens, &tid);
    while (token.kind != token_end_scope) {
        Ast function_scope_statement = ast_function_scope_statement_parse(tokens, &tid, file);
        token = token_get(tokens, &tid);
        bool exit_scope = false;
        if (token.kind != token_end_statement) {
            if (function_scope_statement.kind != ast_invalid) log_error_token(file, token, "Expected end statement");
            Token scope_stack[2048];
            u64 scope_level = 1;
            while (true) {
                if (token.kind == token_begin_scope) {
                    scope_stack[scope_level++ % 2048] = token;
                }
                if (token.kind == token_end_scope) {
                    scope_level--;
                    if (scope_level == 0) {
                        exit_scope = true;
                        break;
                    }
                }
                if (token.kind == token_end_file) {
                    for (u64 i = scope_level; i > 1; i--) {
                        Token last_scope = scope_stack[(scope_level - 1) % 2048];
                        log_error_token(file, last_scope, "Scope is not closed");
                    }
                    break;
                }
                if (token.kind == token_end_statement && scope_level == 1) break;
                token_next(tokens, &tid);
                token = token_get(tokens, &tid);
            }
        }
        if (exit_scope) break;
        if (token.kind == token_end_file) break;
        massert(token.kind == token_end_statement, str("Expected end statement"));

        if (function_scope_statement.kind != ast_invalid) {
            ptr_append(ast.function_scope.statements, ast.function_scope.count, statement_capacity, function_scope_statement);
        }

        token_next(tokens, &tid);
        token = token_get(tokens, &tid);
    }

    token = token_get(tokens, &tid);
    massert(token.kind == token_end_scope, str("Expected end scope"));
    token_next(tokens, &tid);

    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_variable_parse(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Ast ast = {0};
    ast.kind = ast_variable;
    ast.tokens.data = tokens.data + *i;
    ast.file = file;

    Token token = token_get(tokens, &tid);
    ast_expect(token, token_identifier, file);
    ast.variable.name = token.content;
    token_next(tokens, &tid);

    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_expression_value_parse_identifier(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    return ast_variable_parse(tokens, i, file);
}

Ast ast_expression_value_parse_float(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Ast ast = {0};
    ast.kind = ast_float;
    ast.tokens.data = tokens.data + *i;
    ast.file = file;

    Token token = token_get(tokens, &tid);
    ast_expect(token, token_float, file);
    ast.float_value.value = token_float_value(token);
    token_next(tokens, &tid);

    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_expression_value_parse_int(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Ast ast = {0};
    ast.kind = ast_int;
    ast.tokens.data = tokens.data + *i;
    ast.file = file;

    Token token = token_get(tokens, &tid);
    ast_expect(token, token_int, file);
    ast.int_value.value = token_int_value(token);
    token_next(tokens, &tid);

    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_expression_value_parse_subtract(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Ast ast = {0};
    ast.kind = ast_subtract;
    ast.tokens.data = tokens.data + *i;
    ast.file = file;

    Token token = token_get(tokens, &tid);
    ast_expect(token, token_subtract, file);
    token_next(tokens, &tid);

    Ast rhs = ast_expression_value_parse(tokens, &tid, file);
    if (rhs.kind == ast_invalid) return (Ast){0};
    if (rhs.kind == ast_subtract) {
        log_error_token(file, token, "Cannot negate multiple times");
        return (Ast){0};
    }

    Ast implicit_zero = {0};
    implicit_zero.kind = ast_int;
    implicit_zero.int_value.value = 0;
    implicit_zero.tokens.data = ast.tokens.data;
    implicit_zero.file = file;
    implicit_zero.tokens.count = 1;

    ast.biop.lhs = cap_alloc(sizeof(Ast));
    *ast.biop.lhs = implicit_zero;
    ast.biop.rhs = cap_alloc(sizeof(Ast));
    *ast.biop.rhs = rhs;

    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_expression_value_parse(Tokens tokens, u64* i, Cap_File* file) {
    Token token = token_get(tokens, i);
    switch (token.kind) {
        case token_int: {
            return ast_expression_value_parse_int(tokens, i, file);
        }
        case token_float: {
            return ast_expression_value_parse_float(tokens, i, file);
        }
        case token_identifier: {
            return ast_expression_value_parse_identifier(tokens, i, file);
        }
        case token_begin_scope: {
            massert(false, str("Not implemented"));
            return (Ast){0};
        }
        case token_subtract: {
            return ast_expression_value_parse_subtract(tokens, i, file);
        }
        case token_paren_open: {
            Token_Kind delimiters[] = {token_paren_close};
            return ast_expression_parse(tokens, i, file, delimiters, arr_len(delimiters));
        }
        case token_paren_close:
        case token_bitwise_xor:
        case token_shift_left_assign:
        case token_shift_right_assign:
        case token_bitwise_and_assign:
        case token_bitwise_or_assign:
        case token_bitwise_xor_assign:
        case token_shift_left:
        case token_shift_right:
        case token_multiply:
        case token_add:
        case token_divide:
        case token_modulo:
        case token_greater:
        case token_less:
        case token_greater_equal:
        case token_less_equal:
        case token_logical_and:
        case token_bitwise_and:
        case token_logical_or:
        case token_bitwise_or:
        case token_assign:
        case token_arrow:
        case token_equal:
        case token_not_equal:
        case token_not:
        case token_add_assign:
        case token_subtract_assign:
        case token_multiply_assign:
        case token_divide_assign:
        case token_modulo_assign:
        case token_end_statement:
        case token_end_scope:
        case token_program:
        case token_invalid:
        case token_end_file:
        case token_return:
        case token_comma:
        case token_if: {
            log_error_token(file, token, "Unexpected token in expression value parse");
            return (Ast){0};
        }
    }
}

Ast ast_expression_reference_parse(Tokens tokens, u64* i, Cap_File* file, Ast* lhs, Token_Kind* delimiter, u64 delimiter_count) {
    u64 tid = *i;
    Token token = token_get(tokens, &tid);
    ast_expect(token, token_multiply, file);
    Ast ast = {0};
    ast.kind = ast_reference;
    ast.tokens.data = lhs->tokens.data;
    ast.file = file;

    token_next(tokens, &tid);
    token = token_get(tokens, &tid);

    u64 operator_precedence = token_precedence(token.kind);
    bool found_delimiter = false;
    for (u64 i = 0; i < delimiter_count; i++) {
        if (token.kind == delimiter[i]) found_delimiter = true;
    }
    if (!found_delimiter && operator_precedence == UINT64_MAX) return (Ast){0};

    ast.reference.value = cap_alloc(sizeof(Ast));
    *ast.reference.value = *lhs;

    ast.tokens.count = lhs->tokens.count + tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_expression_dereference_parse(Tokens tokens, u64* i, Cap_File* file, Ast* lhs, Token_Kind* delimiter, u64 delimiter_count) {
    u64 tid = *i;
    Token token = token_get(tokens, &tid);
    ast_expect(token, token_multiply, file);
    Ast ast = {0};
    ast.kind = ast_dereference;
    ast.tokens.data = lhs->tokens.data;
    ast.file = file;

    token_next(tokens, &tid);
    token = token_get(tokens, &tid);

    u64 operator_precedence = token_precedence(token.kind);
    bool found_delimiter = false;
    for (u64 i = 0; i < delimiter_count; i++) {
        if (token.kind == delimiter[i]) found_delimiter = true;
    }
    if (!found_delimiter && operator_precedence == UINT64_MAX) return (Ast){0};

    ast.dereference.value = cap_alloc(sizeof(Ast));
    *ast.dereference.value = *lhs;

    ast.tokens.count = lhs->tokens.count + tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast_Kind ast_token_type_to_biop_kind(Token_Kind kind) {
    switch (kind) {
        case token_add:
            return ast_add;
        case token_subtract:
            return ast_subtract;
        case token_multiply:
            return ast_multiply;
        case token_divide:
            return ast_divide;
        case token_modulo:
            return ast_modulo;
        case token_greater:
            return ast_greater;
        case token_less:
            return ast_less;
        case token_greater_equal:
            return ast_greater_equal;
        case token_less_equal:
            return ast_less_equal;
        case token_logical_and:
            return ast_logical_and;
        case token_bitwise_and:
            return ast_bitwise_and;
        case token_logical_or:
            return ast_logical_or;
        case token_bitwise_or:
            return ast_bitwise_or;
        case token_shift_left:
            return ast_shift_left;
        case token_shift_right:
            return ast_shift_right;
        default: {
            mabort(str("Token kind is not a biop"));
            return ast_invalid;
        }
    }
}
Ast _ast_expression_parse(Tokens tokens, u64* i, Cap_File* file, Token_Kind* delimiter, u64 delimiter_count, u64 precedence) {
    u64 tid = *i;
    Ast lhs = ast_expression_value_parse(tokens, &tid, file);
    if (lhs.kind == ast_invalid) return (Ast){0};
    while (true) {
        Token token = token_get(tokens, &tid);
        bool found_delimiter = false;
        for (u64 i = 0; i < delimiter_count; i++) {
            if (token.kind == delimiter[i]) found_delimiter = true;
        }
        if (found_delimiter) break;

        if (token.kind == token_multiply) {
            Ast ast_dereference = ast_expression_dereference_parse(tokens, &tid, file, &lhs, delimiter, delimiter_count);
            if (ast_dereference.kind != ast_invalid) {
                lhs = ast_dereference;
                continue;
            }
        }
        if (token.kind == token_bitwise_and) {
            Ast ast_reference = ast_expression_reference_parse(tokens, &tid, file, &lhs, delimiter, delimiter_count);
            if (ast_reference.kind != ast_invalid) {
                lhs = ast_reference;
                continue;
            }
        }

        Token op_token = token_get(tokens, &tid);
        u64 operator_precedence = token_precedence(op_token.kind);
        if (operator_precedence == UINT64_MAX) {
            log_error_token(file, op_token, "Expected operator");
            return (Ast){0};
        }
        if (operator_precedence > precedence) break;

        Ast rhs = ast_expression_parse(tokens, &tid, file, delimiter, delimiter_count);

        Ast ast_biop = {0};
        ast_biop.tokens.data = lhs.tokens.data;
        ast_biop.file = file;
        ast_biop.tokens.count = tid - *i;
        ast_biop.kind = ast_token_type_to_biop_kind(token.kind);
        ast_biop.biop.lhs = cap_alloc(sizeof(Ast));
        *ast_biop.biop.lhs = lhs;
        ast_biop.biop.rhs = cap_alloc(sizeof(Ast));
        *ast_biop.biop.rhs = rhs;

        lhs = ast_biop;
    }
    token_set(tokens, i, tid);
    return lhs;
}

Ast ast_expression_parse(Tokens tokens, u64* i, Cap_File* file, Token_Kind* delimiter, u64 delimiter_count) {
    return _ast_expression_parse(tokens, i, file, delimiter, delimiter_count, UINT64_MAX);
}

Ast ast_return_parse(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Ast ast = {0};
    ast.kind = ast_return;
    ast.tokens.data = tokens.data + *i;
    ast.file = file;

    Token token = token_get(tokens, &tid);
    ast_expect(token, token_return, file);
    token_next(tokens, &tid);

    token = token_get(tokens, &tid);
    if (token.kind == token_end_statement) {
        ast.return_.value = NULL;
    } else {
        Token_Kind delimiter[] = {token_end_statement};
        Ast ast_value = ast_expression_parse(tokens, &tid, file, delimiter, arr_len(delimiter));
        if (ast_value.kind == ast_invalid) return (Ast){0};
        ast.return_.value = cap_alloc(sizeof(Ast));
        *ast.return_.value = ast_value;
    }

    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_dereference_type_parse(Tokens tokens, u64* i, Cap_File* file, Ast* lhs) {
    u64 tid = *i;
    Ast ast = {0};
    ast.kind = ast_dereference;
    ast.tokens.data = tokens.data + *i;
    ast.file = file;

    Token token = token_get(tokens, &tid);
    ast_expect(token, token_multiply, file);
    token_next(tokens, &tid);

    ast.dereference.value = cap_alloc(sizeof(Ast));
    *ast.dereference.value = *lhs;
    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

bool ast_can_parse_as_type(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Token token = token_get(tokens, &tid);
    if (token.kind != token_identifier) return false;
    token_next(tokens, &tid);
    while (true) {
        token = token_get(tokens, &tid);
        if (token.kind == token_multiply) {
            token_next(tokens, &tid);
            continue;
        }
        break;
    }
    token_set(tokens, i, tid);
    return true;
}

Ast ast_type_parse(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;

    Ast ast = ast_variable_parse(tokens, &tid, file);
    if (ast.kind == ast_invalid) return (Ast){0};

    while (true) {
        Token token = token_get(tokens, &tid);
        if (token.kind == token_multiply) {
            ast = ast_dereference_type_parse(tokens, &tid, file, &ast);
            continue;
        }
        break;
    }
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_assignee_parse(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Ast ast = {0};
    ast.tokens.data = tokens.data + *i;
    ast.file = file;
    cap_context.log = false;
    Ast type = ast_type_parse(tokens, &tid, file);
    cap_context.log = true;
    Token_Kind delimiter[] = {
        token_end_statement,
        token_assign,
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
    };
    if (type.kind == ast_invalid) {
        ast = ast_expression_parse(tokens, &tid, file, delimiter, arr_len(delimiter));
        if (ast.kind == ast_invalid) {
            // make sure we also log whatever make the type parse fail
            ast_type_parse(tokens, &tid, file);
            return (Ast){0};
        }
    } else {
        Token token = token_get(tokens, &tid);
        if (token.kind == token_identifier) {
            ast.kind = ast_variable_declaration;
            ast.variable_declaration.type = cap_alloc(sizeof(Ast));
            *ast.variable_declaration.type = type;
            ast.variable_declaration.name = token.content;
            token_next(tokens, &tid);
        } else {
            tid = *i;
            ast = ast_expression_parse(tokens, &tid, file, delimiter, arr_len(delimiter));
            if (ast.kind == ast_invalid) return (Ast){0};
        }
    }
    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_assignment_parse(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Ast ast = {0};
    ast.kind = ast_assignment;
    ast.tokens.data = tokens.data + *i;
    ast.file = file;

    u64 assignee_capacity = 8;
    ast.assignment.assignees = cap_alloc(assignee_capacity * sizeof(ast.assignment.assignees[0]));
    ast.assignment.assignees_count = 0;
    while (true) {
        Ast assignee = ast_assignee_parse(tokens, &tid, file);
        if (assignee.kind == ast_invalid) return (Ast){0};
        ptr_append(ast.assignment.assignees, ast.assignment.assignees_count, assignee_capacity, assignee);

        Token token = token_get(tokens, &tid);
        if (token.kind == token_comma) {
            token_next(tokens, &tid);
            continue;
        } else if (token.kind == token_end_statement) {
            break;
        } else if (token_is_assign(token.kind)) {
            break;
        } else {
            log_error_token(file, token, "Expected (comma or assignmet or end_statement)");
            return (Ast){0};
        }
    }
    Token assignment_token = token_get(tokens, &tid);
    u64 assignment_token_id = tid;
    if (assignment_token.kind != token_end_statement) {
        massert(token_is_assign(assignment_token.kind), str("Expected assignment"));
        token_next(tokens, &tid);
        u64 values_capacity = 8;
        ast.assignment.values = cap_alloc(values_capacity * sizeof(ast.assignment.values[0]));
        ast.assignment.values_count = 0;
        while (true) {
            Token_Kind delimiters[] = {token_end_statement, token_comma};
            Ast value = ast_expression_parse(tokens, &tid, file, delimiters, arr_len(delimiters));
            if (value.kind == ast_invalid) return (Ast){0};
            ptr_append(ast.assignment.values, ast.assignment.values_count, values_capacity, value);
            Token token = token_get(tokens, &tid);
            if (token.kind == token_end_statement) break;
            massert(token.kind == token_comma, str("Expected comma"));
            token_next(tokens, &tid);
        }

        if (token_is_assign_and_operator(assignment_token.kind)) {
            if (ast.assignment.values_count != ast.assignment.assignees_count) {
                log_error_token(file, assignment_token, "Cannot use assignment operators(+=, -=, etc) with different number of assignees and values");
                return (Ast){0};
            }
            for (u64 i = 0; i < ast.assignment.values_count; i++) {
                Ast* assignee = &ast.assignment.assignees[i];
                if (assignee->kind == ast_variable_declaration) {
                    log_error_ast(assignee, "Cannot use assignment operators(+=, -=, etc) with variable declarations");
                    return (Ast){0};
                }
                Ast* value = &ast.assignment.values[i];
                Ast ast = {0};
                ast.kind = ast_token_type_to_biop_kind(assignment_token.kind);
                ast.tokens.data = tokens.data + assignment_token_id;
                ast.file = file;
                ast.tokens.count = 1;
                ast.biop.lhs = cap_alloc(sizeof(Ast));
                *ast.biop.lhs = *assignee;
                ast.biop.rhs = cap_alloc(sizeof(Ast));
                *ast.biop.rhs = *value;
            }
        }
    }

    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_function_scope_statement_identifier_parse(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    return ast_assignment_parse(tokens, i, file);
}

Ast ast_function_scope_statement_parse(Tokens tokens, u64* i, Cap_File* file) {
    Token token = token_get(tokens, i);
    switch (token.kind) {
        case token_identifier: {
            return ast_function_scope_statement_identifier_parse(tokens, i, file);
        }
        case token_paren_open:
        case token_subtract:
        case token_int:
        case token_float: {
            return ast_assignment_parse(tokens, i, file);
        }
        case token_return: {
            return ast_return_parse(tokens, i, file);
        }
        case token_if: {
            massert(false, str("Not implemented"));
            return (Ast){0};
        }
        case token_begin_scope: {
            massert(false, str("Not implemented"));
            return (Ast){0};
        }
        case token_end_statement: {
            return (Ast){0};
        }
        case token_paren_close:
        case token_bitwise_xor:
        case token_shift_left_assign:
        case token_shift_right_assign:
        case token_bitwise_and_assign:
        case token_bitwise_or_assign:
        case token_bitwise_xor_assign:
        case token_divide:
        case token_shift_left:
        case token_modulo:
        case token_add:
        case token_shift_right:
        case token_greater:
        case token_less:
        case token_greater_equal:
        case token_less_equal:
        case token_logical_and:
        case token_bitwise_and:
        case token_logical_or:
        case token_bitwise_or:
        case token_assign:
        case token_arrow:
        case token_equal:
        case token_not_equal:
        case token_not:
        case token_add_assign:
        case token_subtract_assign:
        case token_multiply_assign:
        case token_divide_assign:
        case token_modulo_assign:
        case token_end_scope:
        case token_multiply:
        case token_program:
        case token_invalid:
        case token_comma:
        case token_end_file: {
            log_error_token(file, token, "Unexpected token in function scope");
            return (Ast){0};
        }
    }
}

bool ast_parse_as_function_declaration(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Token token;
    while (true) {
        if (!ast_can_parse_as_type(tokens, &tid, file)) return false;
        token = token_get(tokens, &tid);
        if (token.kind == token_comma) {
            token_next(tokens, &tid);
            continue;
        }
        break;
    }
    if (token.kind != token_identifier) return false;
    token_next(tokens, &tid);

    token = token_get(tokens, &tid);
    if (token.kind != token_paren_open) return false;
    token_next(tokens, &tid);

    while (true) {
        token = token_get(tokens, &tid);
        if (token.kind == token_paren_close) break;
        if (token.kind == token_end_file) return false;
        token_next(tokens, &tid);
    }
    token_next(tokens, &tid);
    token = token_get(tokens, &tid);
    if (token.kind != token_end_statement && token.kind != token_begin_scope) return false;
    return true;
}

Ast ast_function_declaration_parameter_parse(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Ast ast = {0};
    ast.kind = ast_function_declaration_parameter;
    ast.tokens.data = tokens.data + *i;
    ast.file = file;

    Ast type = ast_type_parse(tokens, &tid, file);
    if (type.kind == ast_invalid) return (Ast){0};
    ast.function_declaration_parameters.type = cap_alloc(sizeof(Ast));
    *ast.function_declaration_parameters.type = type;

    Token token = token_get(tokens, &tid);
    ast_expect(token, token_identifier, file);
    ast.function_declaration_parameters.name = token.content;
    token_next(tokens, &tid);

    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_function_declaration_parse(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Ast ast = {0};
    ast.kind = ast_function_declaration;
    ast.tokens.data = tokens.data + *i;
    ast.file = file;

    Token token;

    u64 return_types_capacity = 8;
    ast.function_declaration.return_types = cap_alloc(return_types_capacity * sizeof(ast.function_declaration.return_types[0]));
    ast.function_declaration.return_types_count = 0;

    while (true) {
        Ast type = ast_type_parse(tokens, &tid, file);
        if (type.kind == ast_invalid) return (Ast){0};
        ptr_append(ast.function_declaration.return_types, ast.function_declaration.return_types_count, return_types_capacity, type);
        token = token_get(tokens, &tid);
        if (token.kind == token_comma) {
            token_next(tokens, &tid);
            continue;
        }
        break;
    }

    ast_expect(token, token_identifier, file);
    ast.function_declaration.name = token.content;
    token_next(tokens, &tid);

    token = token_get(tokens, &tid);
    ast_expect(token, token_paren_open, file);
    token_next(tokens, &tid);

    token = token_get(tokens, &tid);
    if (token.kind != token_paren_close) {
        u64 parameters_capacity = 8;
        ast.function_declaration.parameters = cap_alloc(parameters_capacity * sizeof(ast.function_declaration.parameters[0]));
        ast.function_declaration.parameters_count = 0;
        while (true) {
            Ast parameter = ast_function_declaration_parameter_parse(tokens, &tid, file);
            if (parameter.kind == ast_invalid) return (Ast){0};
            ptr_append(ast.function_declaration.parameters, ast.function_declaration.parameters_count, parameters_capacity, parameter);
            token = token_get(tokens, &tid);
            if (token.kind == token_comma) {
                token_next(tokens, &tid);
                continue;
            } else if (token.kind == token_paren_close) break;
            log_error_token(file, token, "Expected comma or close paren");
            return (Ast){0};
        }
    } else {
        ast.function_declaration.parameters = NULL;
        ast.function_declaration.parameters_count = 0;
    }
    massert(token.kind == token_paren_close, str("Expected close paren"));
    token_next(tokens, &tid);

    token = token_get(tokens, &tid);
    if (token.kind == token_begin_scope) {
        Ast body = ast_function_scope_parse(tokens, &tid, file);
        if (body.kind == ast_invalid) return (Ast){0};
        ast.function_declaration.body = cap_alloc(sizeof(Ast));
        *ast.function_declaration.body = body;
    } else ast.function_declaration.body = NULL;

    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_parse_top_level_statement_identifier(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    if (ast_parse_as_function_declaration(tokens, &tid, file)) return ast_function_declaration_parse(tokens, i, file);
    return ast_assignment_parse(tokens, i, file);
}

Ast ast_parse_top_level_statement(Tokens tokens, u64* i, Cap_File* file) {
    Token token = token_get(tokens, i);
    switch (token.kind) {
        case token_program:
            return ast_parse_program(tokens, i, file);
        case token_identifier:
            return ast_parse_top_level_statement_identifier(tokens, i, file);
        case token_end_statement:
            return (Ast){0};
        case token_end_scope:
            return (Ast){0};  // we don't need to error here as it will be caught as a error for not having a begin scope
        case token_paren_open:
        case token_paren_close:
        case token_bitwise_xor:
        case token_shift_left_assign:
        case token_shift_right_assign:
        case token_bitwise_and_assign:
        case token_bitwise_or_assign:
        case token_bitwise_xor_assign:
        case token_subtract:
        case token_divide:
        case token_modulo:
        case token_add:
        case token_shift_left:
        case token_shift_right:
        case token_greater:
        case token_less:
        case token_greater_equal:
        case token_less_equal:
        case token_logical_and:
        case token_bitwise_and:
        case token_logical_or:
        case token_bitwise_or:
        case token_assign:
        case token_arrow:
        case token_equal:
        case token_not_equal:
        case token_not:
        case token_add_assign:
        case token_subtract_assign:
        case token_multiply_assign:
        case token_divide_assign:
        case token_modulo_assign:
        case token_return:
        case token_if:
        case token_end_file:
        case token_multiply:
        case token_begin_scope:
        case token_invalid:
        case token_int:
        case token_float:
        case token_comma:
            log_error_token(file, token, "Unexpected token in top level");
            return (Ast){0};
    }
}

Ast ast_parse_top_level_ast(Tokens tokens, u64* i, Cap_File* file) {
    u64 tid = *i;
    Ast ast = {0};
    ast.kind = ast_top_level;
    ast.tokens.data = tokens.data + *i;
    ast.file = file;

    u64 program_capacity = 8;
    ast.top_level.programs = cap_alloc(program_capacity * sizeof(ast.top_level.programs[0]));
    ast.top_level.programs_count = 0;

    u64 function_capacity = 8;
    ast.top_level.functions = cap_alloc(function_capacity * sizeof(ast.top_level.functions[0]));
    ast.top_level.functions_count = 0;

    Token token = token_get(tokens, &tid);
    while (token.kind != token_end_file) {
        Ast top_level_statement = ast_parse_top_level_statement(tokens, &tid, file);
        switch (top_level_statement.kind) {
            case ast_program:
                ptr_append(ast.top_level.programs, ast.top_level.programs_count, program_capacity, top_level_statement);
                break;
            case ast_function_declaration:
                ptr_append(ast.top_level.functions, ast.top_level.functions_count, function_capacity, top_level_statement);
                break;
            case ast_invalid:
                break;
            case ast_top_level:
            case ast_dereference:
            case ast_reference:
            case ast_variable:
            case ast_function_scope:
            case ast_scope:
            case ast_assignment:
            case ast_variable_declaration:
            case ast_return:
            case ast_int:
            case ast_float:
            case ast_function_declaration_parameter:
            case ast_add:
            case ast_subtract:
            case ast_multiply:
            case ast_divide:
            case ast_modulo:
            case ast_greater:
            case ast_less:
            case ast_greater_equal:
            case ast_less_equal:
            case ast_logical_and:
            case ast_bitwise_and:
            case ast_logical_or:
            case ast_bitwise_or:
            case ast_shift_left:
            case ast_shift_right:
                log_error_token(file, token, "Unexpected top level statement");
                break;
        }
        token = token_get(tokens, &tid);
        if (token.kind != token_end_statement) {
            if (top_level_statement.kind != ast_invalid) log_error_token(file, token, "Expected end statement");
            Token scope_stack[2048];
            u64 scope_level = 0;
            while (true) {
                if (token.kind == token_begin_scope) {
                    scope_stack[scope_level++ % 2048] = token;
                }
                if (token.kind == token_end_scope) {
                    if (scope_level == 0) {
                        log_error_token(file, token, "Closing scope without opening scope");
                    } else {
                        scope_level--;
                    }
                }
                if (token.kind == token_end_file) {
                    for (u64 i = scope_level; i > 1; i--) {
                        Token last_scope = scope_stack[(scope_level - 1) % 2048];
                        log_error_token(file, last_scope, "Scope is not closed");
                    }
                    break;
                }
                if (token.kind == token_end_statement && scope_level == 0) break;
                token_next(tokens, &tid);
                token = token_get(tokens, &tid);
            }
        }
        if (token.kind == token_end_file) break;
        massert(token.kind == token_end_statement, str("Expected end statement"));
        token_next(tokens, &tid);
        token = token_get(tokens, &tid);
    }

    ast.tokens.count = tid - *i;
    token_set(tokens, i, tid);
    return ast;
}

Ast ast_parse_tokens(Tokens tokens, Cap_File* file) {
    u64 tid = 0;
    return ast_parse_top_level_ast(tokens, &tid, file);
}

String ast_kind_to_string(Ast_Kind kind) {
    switch (kind) {
        case ast_invalid:
            return str("invalid");
        case ast_return:
            return str("return");
        case ast_int:
            return str("int");
        case ast_float:
            return str("float");
        case ast_function_declaration:
            return str("function_declaration");
        case ast_function_declaration_parameter:
            return str("function_declaration_parameter");
        case ast_scope:
            return str("scope");
        case ast_assignment:
            return str("assignment");
        case ast_variable_declaration:
            return str("variable_declaration");
        case ast_top_level:
            return str("top_level");
        case ast_program:
            return str("program");
        case ast_function_scope:
            return str("function_scope");
        case ast_variable:
            return str("variable");
        case ast_dereference:
            return str("dereference");
        case ast_reference:
            return str("reference");
        case ast_add:
            return str("add");
        case ast_subtract:
            return str("subtract");
        case ast_multiply:
            return str("multiply");
        case ast_divide:
            return str("divide");
        case ast_modulo:
            return str("modulo");
        case ast_greater:
            return str("greater");
        case ast_less:
            return str("less");
        case ast_greater_equal:
            return str("greater_equal");
        case ast_less_equal:
            return str("less_equal");
        case ast_logical_and:
            return str("logical_and");
        case ast_bitwise_and:
            return str("bitwise_and");
        case ast_logical_or:
            return str("logical_or");
        case ast_bitwise_or:
            return str("bitwise_or");
        case ast_shift_left:
            return str("shift_left");
        case ast_shift_right:
            return str("shift_right");
    }
}

String ast_to_string_short(Ast* ast) {
    String str = {0};
    String kind = ast_kind_to_string(ast->kind);

    Token first_token = ast->tokens.data[0];
    String token_string = token_token_to_string(first_token);

    Token last_token = ast->tokens.data[ast->tokens.count - 1];
    String last_token_string = token_token_to_string(last_token);

    char buffer[2048];
    if (ast->tokens.count == 0) {
        snprintf(buffer, 2048, "%.*s", str_info(kind));
    } else if (ast->tokens.count == 1) {
        snprintf(buffer, 2048, "%.*s(%.*s)", str_info(kind), str_info(token_string));
    } else if (ast->tokens.count == 2) {
        snprintf(buffer, 2048, "%.*s(%.*s, %.*s)", str_info(kind), str_info(token_string), str_info(last_token_string));
    } else {
        snprintf(buffer, 2048, "%.*s(%.*s ... %.*s)", str_info(kind), str_info(token_string), str_info(last_token_string));
    }
    u64 length = strlen(buffer);
    char* ptr = cap_alloc(length + 1);
    memcpy(ptr, buffer, length);
    return string_create(ptr, length);
}

String ast_get_substring(Ast* ast) {
    String str = {0};
    if (ast->tokens.count == 0) {
        massert(false, str("all asts should have tokens"));
        return str;
    }
    str.data = ast->tokens.data[0].content.data;
    Token* last_token = &ast->tokens.data[ast->tokens.count - 1];
    char* end_ptr = last_token->content.data + last_token->content.length;
    str.length = end_ptr - str.data;
    return str;
}
