#include "cap.h"
#include "cap/tokens.h"

bool ast_can_interpret_as_type(Token** token_start) {
    Token* token = *token_start;
    if (token->type != tt_id) return false;
    token++;

    while (true) {
        if (token->type == tt_mul) {
            token++;
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

    while (true) {
        if (token->type == tt_mul) {
            token++;
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

Ast ast_expression_value_parse(Token** tokens) {
    Token* token = *tokens;
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
            return ast_variable_parse(tokens);
        }
        default: {
            log_error_token(token, "expected token parsable av expression value");
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

Ast ast_expression_parse(Token** tokens, TokenType* delimiters, u32 num_delimiters) {
    Token* token = *tokens;
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;
    ast.kind = ast_expression;

    Ast lhs = ast_expression_value_parse(&token);
    while (true) {
        bool found_delimiter = false;
        for (u32 i = 0; i < num_delimiters; i++) {
            if (token->type == delimiters[i]) {
                found_delimiter = true;
                break;
            }
        }
        if (found_delimiter) break;

        if (token->type == tt_dot) {
            lhs = ast_value_access_parse(&token, &lhs);
            continue;
        }

        // TODO: biops and order of operations
        massert(false, "not implemented");
    }

    ast.expression.value = alloc(sizeof(Ast));
    *ast.expression.value = lhs;
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

    if (token->type != tt_lparen) {
        log_error_token(token, "expected '(' for alloc");
        return err;
    }
    token++;

    Ast type = ast_type_parse(&token);
    if (type.kind == ast_invalid) return type;
    ast.alloc.type = alloc(sizeof(Ast));
    *ast.alloc.type = type;

    if (token->type == tt_comma) {
        token++;
        TokenType delimiters[] = {tt_rparen, tt_comma};
        Ast count = ast_expression_parse(&token, delimiters, arr_length(delimiters));
        if (count.kind == ast_invalid) return count;
        if (token->type == tt_comma) {
            log_error_token(token, "alloc should only take at most 2 parameters");
            return err;
        }
        ast.alloc.count = alloc(sizeof(Ast));
        *ast.alloc.count = count;
    } else {
        ast.alloc.count = NULL;
    }

    if (token->type != tt_rparen) {
        log_error_token(token, "expected closing ')' for alloc");
        return err;
    }
    token++;

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
