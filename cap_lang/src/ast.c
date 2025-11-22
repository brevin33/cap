#include "cap.h"
#include "cap/tokens.h"

bool ast_can_interpret_as_allocator(Token** tokens) {
    Token* token = *tokens;
    if (token->type != tt_at) return false;
    token++;
    if (token->type != tt_id) return false;
    token++;
    if (token->type == tt_dot) {
        token++;
        if (token->type != tt_id && token->type != tt_int) return false;
        token++;
    }
    *tokens = token;
    return true;
}

bool ast_can_interpret_as_type_with_allocator(Token** tokens) {
    Token* token = *tokens;
    Token* token_start = token;
    bool res = ast_can_interpret_as_type(&token);
    Token* t = token_start;
    bool found_allocator = false;
    while (t != token) {
        if (t->type == tt_at) found_allocator = true;
        t++;
    }
    *tokens = token;
    return res && found_allocator;
}

bool ast_can_interpret_as_type(Token** token_start) {
    Token* token = *token_start;
    if (token->type != tt_id) return false;
    token++;

    while (true) {
        if (token->type == tt_mul) {
            token++;
            ast_can_interpret_as_allocator(&token);
            continue;
        }
        break;
    }

    *token_start = token;
    return true;
}

bool ast_interpret_ast_assignment(Token* token) {
    while (token->type != tt_end_statement) {
        if (token->type == tt_equals) {
            return true;
        }
        if (token->type == tt_eof) return false;
        token++;
    }
    return false;
}

bool ast_interpret_as_variable_declaration(Token* token) {
    if (!ast_can_interpret_as_type(&token)) return false;

    if (token->type != tt_id) return false;
    token++;

    return true;
}

bool ast_interpret_as_function_declaration(Token* token) {
    if (!ast_can_interpret_as_type(&token)) return false;

    if (token->type != tt_id) return false;
    token++;

    if (token->type != tt_lparen) return false;
    token++;

    while (true) {
        if (token->type == tt_rparen) break;
        if (token->type == tt_eof) return false;
        token++;
    }

    return true;
}

Ast ast_from_tokens(Token* token_start) {
    Token* token = token_start;
    Ast ast = {0};
    ast.token_start = token_start;
    ast.kind = ast_top_level;

    while (token->type != tt_eof) {
        Ast val = ast_general_parse(&token);
        switch (val.kind) {
            case ast_function_declaration: {
                Ast_List_add(&ast.top_level.functions, &val);
                break;
            }
            case ast_program: {
                Ast_List_add(&ast.top_level.programs, &val);
                break;
            }
            case ast_invalid: {
                while (true) {
                    if (token->type == tt_lbrace) {
                        u32 num_braces = 1;
                        Token* opening_brace = token;
                        while (true) {
                            if (token->type == tt_lbrace) num_braces++;
                            if (token->type == tt_rbrace) num_braces--;
                            if (token->type == tt_eof) {
                                log_error_token(opening_brace, "unterminated block");
                                ast.num_tokens = token - ast.token_start;
                                return ast;
                            }
                            if (num_braces == 0) break;
                            token++;
                        }
                        massert(token->type == tt_rbrace, "should have found a '}' for block");
                        token++;
                        continue;
                    }
                    if (token->type == tt_eof) break;
                    if (token->type == tt_end_statement) break;
                    token++;
                }
                break;
            }
            default: {
                log_error_ast(&ast, "not a valid ast in the top level");
                break;
            }
        }
        massert(token->type == tt_end_statement, "should have found a tt_end_statement for after statement");
        token++;
    }

    ast.num_tokens = token - token_start;
    return ast;
}

Ast ast_type_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_type;

    if (token->type != tt_id) {
        log_error_token(token, "expected type name");
        return err;
    }
    ast.type.name = token_get_id(token);
    token++;

    cap_context.log_errors = false;
    Ast allocator = ast_allocator_parse(&token);
    ast.type.base_allocator = alloc(sizeof(Ast));
    *ast.type.base_allocator = allocator;
    cap_context.log_errors = true;

    while (true) {
        if (token->type == tt_mul) {
            token++;

            // we ignore the error here because an allocator doesn't have to be specified
            cap_context.log_errors = false;
            Ast allocator = ast_allocator_parse(&token);
            Ast_List_add(&ast.type.ptr_allocators, &allocator);
            cap_context.log_errors = true;

            ast.type.ptr_count++;
            continue;
        }
        break;
    }

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_parameter_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_declaration_parameter;

    Ast type = ast_type_parse(&token);
    if (type.kind == ast_invalid) return type;
    ast.declaration_parameter.type = alloc(sizeof(Ast));
    *ast.declaration_parameter.type = type;

    if (token->type != tt_id) {
        log_error_token(token, "expected parameter name");
        return err;
    }
    ast.declaration_parameter.name = token_get_id(token);
    token++;

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_parameter_list_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_declaration_parameter_list;

    if (token->type != tt_lparen) {
        log_error_token(token, "expected '(' for parameter list");
        return err;
    }
    token++;

    if (token->type != tt_rparen) {
        while (true) {
            Ast parameter = ast_parameter_parse(&token);
            if (parameter.kind == ast_invalid) return parameter;
            Ast_List_add(&ast.declaration_parameter_list.parameters, &parameter);

            if (token->type == tt_rparen) break;
            if (token->type == tt_comma) {
                token++;
                continue;
            }
            log_error_token(token, "expected ',' or ')' for after parameter");
            return err;
        }
    }
    massert(token->type == tt_rparen, "should have found a ')' for parameter list");
    token++;

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_variable_declaration_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_variable_declaration;

    Ast type = ast_type_parse(&token);
    if (type.kind == ast_invalid) return type;
    ast.variable_declaration.type = alloc(sizeof(Ast));
    *ast.variable_declaration.type = type;

    if (token->type != tt_id) {
        log_error_token(token, "expected variable name");
        return err;
    }
    Token* variable_name = token;
    ast.variable_declaration.name = token_get_id(token);
    token++;

    if (token->type == tt_equals) {
        TokenType delimiters[] = {tt_equals};
        Ast expr = ast_expression_parse(&variable_name, delimiters, arr_length(delimiters));

        if (expr.kind == ast_invalid) return err;
        Ast assignment = ast_assignment_parse(&token, &expr);
        if (assignment.kind == ast_invalid) return err;
        ast.variable_declaration.assignment = alloc(sizeof(Ast));
        *ast.variable_declaration.assignment = assignment;
    } else {
        ast.variable_declaration.assignment = NULL;
    }

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_int_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_int;

    if (token->type != tt_int) {
        log_error_token(token, "expected int");
        return err;
    }
    ast.int_.value = token_get_id(token);
    token++;

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_float_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_float;

    if (token->type != tt_float) {
        log_error_token(token, "expected float");
        return err;
    }
    ast.float_.value = token_get_float(token);
    token++;

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_variable_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_variable;

    if (token->type != tt_id) {
        log_error_token(token, "expected variable name");
        return err;
    }
    ast.variable.name = token_get_id(token);
    token++;

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_expression_value_parse(Token** tokens, TokenType* delimiters, u32 num_delimiters) {
    Token* token = *tokens;
    Token* token_start = token;
    switch (token->type) {
        case tt_int: {
            return ast_int_parse(tokens);
        }
        case tt_float: {
            return ast_float_parse(tokens);
        }
        case tt_id: {
            char* id = token_get_id(token);
            if (strcmp(id, "alloc") == 0) {
                return ast_alloc_parse(tokens);
            }
            token++;
            if (token->type == tt_lparen) {
                return ast_function_call_parse(tokens);
            }

            if (ast_can_interpret_as_type_with_allocator(&token_start)) return ast_type_parse(tokens);
            return ast_variable_parse(tokens);
        }
        default: {
            log_error_token(token, "expected token parsable as expression value");
            Ast err = {0};
            return err;
        }
    }
}

Ast ast_return_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_return;

    if (token->type != tt_return) {
        log_error_token(token, "expected return");
        return err;
    }
    token++;

    TokenType delimiters[] = {tt_end_statement};
    Ast value = ast_expression_parse(&token, delimiters, arr_length(delimiters));
    if (value.kind == ast_invalid) return value;
    ast.return_.value = alloc(sizeof(Ast));
    *ast.return_.value = value;

    if (token->type != tt_end_statement) {
        log_error_token(token, "expected end statement after return");
        return err;
    }

    ast.num_tokens = token - ast.token_start;
    *tokens = token;
    return ast;
}

Ast ast_get_parse(Token** tokens, Ast* lhs) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = lhs->token_start;
    ast.kind = ast_get;
    ast.get.expression = alloc(sizeof(Ast));
    *ast.get.expression = *lhs;
    if (token->type != tt_mul) {
        log_error_token(token, "expected '*' for get");
        return err;
    }
    token++;
    ast.num_tokens = token - ast.token_start;
    *tokens = token;
    return ast;
}

Ast ast_ptr_parse(Token** tokens, Ast* lhs) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = lhs->token_start;
    ast.kind = ast_ptr;
    ast.ptr.expression = alloc(sizeof(Ast));
    *ast.ptr.expression = *lhs;
    if (token->type != tt_bit_and) {
        log_error_token(token, "expected '&' for ptr");
        return err;
    }
    token++;
    ast.num_tokens = token - ast.token_start;
    *tokens = token;
    return ast;
}

Ast ast_value_access_parse(Token** tokens, Ast* lhs) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = lhs->token_start;
    ast.kind = ast_value_access;
    ast.value_access.expr = alloc(sizeof(Ast));
    *ast.value_access.expr = *lhs;
    if (token->type != tt_dot) {
        log_error_token(token, "expected '.' for value access");
        return err;
    }
    token++;
    if (token->type != tt_id) {
        log_error_token(token, "expected identifier after '.'");
        return err;
    }
    ast.value_access.access_name = token_get_id(token);
    token++;
    ast.num_tokens = token - ast.token_start;
    *tokens = token;
    return ast;
}

u32 ast_token_precedence(TokenType type) {
    switch (type) {
        case tt_mul:
        case tt_div:
        case tt_mod:
            return 3;
        case tt_add:
        case tt_sub:
            return 4;
        case tt_bit_shl:
        case tt_bit_shr:
            return 5;
        case tt_less_than:
        case tt_greater_than:
        case tt_less_than_equals:
        case tt_greater_than_equals:
            return 6;
        case tt_equals_equals:
        case tt_not_equals:
            return 7;
        case tt_bit_and:
            return 8;
        case tt_bit_xor:
            return 9;
        case tt_bit_or:
            return 10;
        case tt_and:
            return 11;
        case tt_or:
            return 12;
        case tt_dot:
            return 20;
        default:
            return UINT32_MAX;
    }
}

Ast_Kind ast_get_biop_kind(TokenType type) {
    switch (type) {
        case tt_add:
            return ast_add;
        case tt_sub:
            return ast_sub;
        case tt_mul:
            return ast_mul;
        case tt_div:
            return ast_div;
        case tt_mod:
            return ast_mod;
        case tt_bit_and:
            return ast_bit_and;
        case tt_bit_or:
            return ast_bit_or;
        case tt_bit_xor:
            return ast_bit_xor;
        case tt_bit_not:
            return ast_bit_not;
        case tt_bit_shl:
            return ast_bit_shl;
        case tt_bit_shr:
            return ast_bit_shr;
        case tt_and:
            return ast_and;
        case tt_or:
            return ast_or;
        case tt_equals_equals:
            return ast_equals_equals;
        case tt_not_equals:
            return ast_not_equals;
        case tt_less_than:
            return ast_less_than;
        case tt_greater_than:
            return ast_greater_than;
        case tt_less_than_equals:
            return ast_less_than_equals;
        case tt_greater_than_equals:
            return ast_greater_than_equals;
        default:
            massert(false, "should never happen");
            return ast_invalid;
    }
}

Ast _ast_expression_parse(Token** tokens, TokenType* delimiters, u32 num_delimiters, u32 precedence) {
    Token* token = *tokens;
    Ast err = {0};
    Token* start_token = token;
    Ast lhs = ast_expression_value_parse(&token, delimiters, num_delimiters);
    while (true) {
        bool found_delimiter = false;
        for (u32 i = 0; i < num_delimiters; i++) {
            if (token->type == delimiters[i]) {
                found_delimiter = true;
                break;
            }
        }
        if (found_delimiter) break;

        u32 op_precedence = ast_token_precedence(token->type);
        if (op_precedence == UINT32_MAX) {
            log_error_token(token, "expected binary operator");
            return err;
        }
        if (op_precedence >= precedence) break;

        // special cases
        if (token->type == tt_dot) {
            lhs = ast_value_access_parse(&token, &lhs);
            continue;
        }
        if (token->type == tt_mul) {
            Token* next_token = token + 1;
            bool found_delimiter = false;
            for (u32 i = 0; i < num_delimiters; i++) {
                if (next_token->type == delimiters[i]) {
                    found_delimiter = true;
                    break;
                }
            }
            u32 next_token_precedence = ast_token_precedence(next_token->type);
            if (next_token_precedence != UINT32_MAX || found_delimiter) {
                lhs = ast_get_parse(&token, &lhs);
                if (lhs.kind == ast_invalid) return lhs;
                continue;
            }
        }
        if (token->type == tt_bit_and) {
            Token* next_token = token + 1;
            bool found_delimiter = false;
            for (u32 i = 0; i < num_delimiters; i++) {
                if (next_token->type == delimiters[i]) {
                    found_delimiter = true;
                    break;
                }
            }
            u32 next_token_precedence = ast_token_precedence(next_token->type);
            if (next_token_precedence != UINT32_MAX || found_delimiter) {
                lhs = ast_ptr_parse(&token, &lhs);
                if (lhs.kind == ast_invalid) return lhs;
                continue;
            }
        }
        token++;

        Ast rhs = _ast_expression_parse(&token, delimiters, num_delimiters, op_precedence);
        Ast_Kind biop_kind = ast_get_biop_kind(token->type);

        Ast biop = {0};
        biop.kind = biop_kind;

        biop.biop.lhs = alloc(sizeof(Ast));
        *biop.biop.lhs = lhs;

        biop.biop.rhs = alloc(sizeof(Ast));
        *biop.biop.rhs = rhs;

        lhs = biop;
    }

    lhs.token_start = start_token;
    lhs.num_tokens = token - start_token;
    *tokens = token;
    return lhs;
}

Ast ast_expression_parse(Token** tokens, TokenType* delimiters, u32 num_delimiters) {
    Token* token = *tokens;
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_expression;

    Ast expr = _ast_expression_parse(&token, delimiters, num_delimiters, UINT32_MAX - 1);
    if (expr.kind == ast_invalid) return expr;
    ast.expression.value = alloc(sizeof(Ast));
    *ast.expression.value = expr;

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_assignment_parse(Token** tokens, Ast* assignee) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = assignee->token_start;
    ast.kind = ast_assignment;
    ast.assignment.assignee = alloc(sizeof(Ast));
    *ast.assignment.assignee = *assignee;

    if (token->type != tt_equals) {
        log_error_token(token, "expected '=' for assignment");
        return err;
    }
    token++;

    TokenType delimiters[] = {tt_end_statement};
    Ast value = ast_expression_parse(&token, delimiters, arr_length(delimiters));
    if (value.kind == ast_invalid) return value;
    ast.assignment.value = alloc(sizeof(Ast));
    *ast.assignment.value = value;

    if (token->type != tt_end_statement) {
        log_error_token(token, "expected end statement after assignment");
        return err;
    }

    ast.num_tokens = token - ast.token_start;
    *tokens = token;
    return ast;
}

Ast ast_return_parse(Token** tokens);

Ast ast_body_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_body;

    if (token->type != tt_lbrace) {
        log_error_token(token, "expected '{' for body");
        return err;
    }
    Token* opening_brace = token;
    token++;

    if (token->type != tt_rbrace) {
        while (true) {
            Ast statement = ast_general_parse(&token);
            if (statement.kind == ast_invalid) {
                while (true) {
                    if (token->type == tt_lbrace) {
                        u32 num_braces = 1;
                        Token* opening_brace = token;
                        while (true) {
                            if (token->type == tt_lbrace) num_braces++;
                            if (token->type == tt_rbrace) num_braces--;
                            if (token->type == tt_eof) {
                                ast.num_tokens = token - ast.token_start;
                                *tokens = token;
                                return ast;
                            }
                            if (num_braces == 0) break;
                            token++;
                        }
                        massert(token->type == tt_rbrace, "should have found a '}' for block");
                        token++;
                        continue;
                    }
                    if (token->type == tt_eof) {
                        log_error_token(opening_brace, "unterminated block");
                        ast.num_tokens = token - ast.token_start;
                        return ast;
                    }
                    if (token->type == tt_end_statement) break;
                    token++;
                }
            }
            Ast_List_add(&ast.body.statements, &statement);
            massert(token->type == tt_end_statement, "should have found a tt_end_statement for after statement");
            token++;

            if (token->type == tt_rbrace) break;
            if (token->type == tt_eof) {
                log_error_token(opening_brace, "unterminated block");
                ast.num_tokens = token - ast.token_start;
                return ast;
            }
        }
    }
    massert(token->type == tt_rbrace, "should have found a '}' for body");
    token++;

    if (token->type != tt_end_statement) {
        log_error_token(token, "expected end statement after body");
        return err;
    }

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_program_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_program;

    if (token->type != tt_program) {
        log_error_token(token, "expected program token when parsing program");
        return err;
    }
    token++;

    if (token->type != tt_id) {
        log_error_token(token, "expected program name when parsing program");
        return err;
    }
    ast.program.name = token_get_id(token);
    token++;

    Ast body = ast_body_parse(&token);
    if (body.kind == ast_invalid) return body;
    ast.program.body = alloc(sizeof(Ast));
    *ast.program.body = body;

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_function_declaration_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_function_declaration;

    Ast return_type = ast_type_parse(&token);
    if (return_type.kind == ast_invalid) return return_type;
    ast.function_declaration.return_type = alloc(sizeof(Ast));
    *ast.function_declaration.return_type = return_type;

    if (token->type != tt_id) {
        log_error_token(token, "expected function name");
        return err;
    }
    ast.function_declaration.name = token_get_id(token);
    token++;

    Ast parameter_list = ast_parameter_list_parse(&token);
    if (parameter_list.kind == ast_invalid) return parameter_list;
    ast.function_declaration.parameter_list = alloc(sizeof(Ast));
    *ast.function_declaration.parameter_list = parameter_list;

    if (token->type == tt_lbrace) {
        Ast body = ast_body_parse(&token);
        if (body.kind == ast_invalid) return body;
        ast.function_declaration.body = alloc(sizeof(Ast));
        *ast.function_declaration.body = body;
    } else {
        ast.function_declaration.body = NULL;
    }

    if (token->type != tt_end_statement) {
        log_error_token(token, "expected end statement after function declaration");
        return err;
    }

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_function_call_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_function_call;

    if (token->type != tt_id) {
        log_error_token(token, "expected function name");
        return err;
    }
    ast.function_call.name = token_get_id(token);
    token++;

    ast.function_call.parameters = alloc(sizeof(Ast));
    *ast.function_call.parameters = ast_function_call_parameter_parse(&token);
    if (ast.function_call.parameters->kind == ast_invalid) return ast;

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_allocator_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_allocator;

    if (token->type != tt_at) {
        log_error_token(token, "expected allocator");
        return err;
    }
    token++;

    if (token->type != tt_id) {
        log_error_token(token, "expected allocator name");
        return err;
    }
    ast.allocator.variable_name = token_get_id(token);
    token++;

    if (token->type == tt_dot) {
        token++;
        if (token->type != tt_id && token->type != tt_int) {
            log_error_token(token, "expected field name");
            return err;
        }
        ast.allocator.field_name = token_get_id(token);
        token++;
    } else {
        ast.allocator.field_name = NULL;
    }

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_function_call_parameter_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_function_call_parameter;

    if (token->type != tt_lparen) {
        log_error_token(token, "expected '(' for function call parameter");
        return err;
    }
    token++;

    Ast_List parameters = {0};

    if (token->type != tt_rparen) {
        while (true) {
            TokenType delimiters[] = {tt_rparen, tt_comma};
            Ast parameter = ast_expression_parse(&token, delimiters, arr_length(delimiters));
            if (parameter.kind == ast_invalid) return parameter;
            Ast_List_add(&parameters, &parameter);

            if (token->type == tt_rparen) break;
            if (token->type == tt_comma) {
                token++;
                continue;
            }
        }
    }
    massert(token->type == tt_rparen, "should have found a ')' for function call parameter");
    token++;

    ast.function_call_parameter.parameters = parameters;

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_alloc_parse(Token** tokens) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_alloc;

    massert(token->type == tt_id, "should have found an alloc");
    massert(strcmp(token_get_id(token), "alloc") == 0, "should have found an alloc");
    token++;

    Ast parameter_list = ast_function_call_parameter_parse(&token);
    if (parameter_list.kind == ast_invalid) return parameter_list;
    ast.alloc.parameters = alloc(sizeof(Ast));
    *ast.alloc.parameters = parameter_list;

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
}

Ast ast_general_id_parse(Token** tokens) {
    if (ast_interpret_as_function_declaration(*tokens)) {
        return ast_function_declaration_parse(tokens);
    }
    if (ast_interpret_as_variable_declaration(*tokens)) {
        return ast_variable_declaration_parse(tokens);
    }
    if (ast_interpret_ast_assignment(*tokens)) {
        Token* token = *tokens;
        TokenType delimiters[] = {tt_end_statement, tt_equals};
        Ast assignee = ast_expression_parse(&token, delimiters, arr_length(delimiters));
        if (assignee.kind == ast_invalid) return assignee;
        *tokens = token;
        return ast_assignment_parse(tokens, &assignee);
    }
    TokenType delimiters[] = {tt_end_statement};
    return ast_expression_parse(tokens, delimiters, arr_length(delimiters));
}

Ast ast_general_parse(Token** tokens) {
    switch ((*tokens)->type) {
        case tt_id: {
            return ast_general_id_parse(tokens);
        }
        case tt_return: {
            return ast_return_parse(tokens);
        }
        case tt_program: {
            return ast_program_parse(tokens);
        }
        default: {
            break;
        }
    }
    massert(false, "should never happen");
    Ast error_ast = {0};
    return error_ast;
}
