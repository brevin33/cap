#include "cap.h"

bool ast_can_interpret_as_type(Token** token_start) {
    Token* token = *token_start;
    if (token->type != tt_id) return false;
    token++;
    *token_start = token;
    return true;
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
        Ast* expr = alloc(sizeof(Ast));
        *expr = ast_expression_parse(&variable_name, delimiters, arr_length(delimiters));

        if (expr->kind == ast_invalid) return err;
        Ast assignment = ast_assignment_parse(&token, expr);
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
    Ast err = {0};
    Ast ast = {0};
    ast.token_start = token;

    switch (token->type) {
        case tt_int: {
            ast = ast_int_parse(&token);
            if (ast.kind == ast_invalid) return err;
            break;
        }
        case tt_float: {
            ast = ast_float_parse(&token);
            if (ast.kind == ast_invalid) return err;
            break;
        }
        case tt_id: {
            ast = ast_variable_parse(&token);
            if (ast.kind == ast_invalid) return err;
            break;
        }
        default: {
            log_error_token(token, "expected token parsable av expression value");
            return err;
        }
    }

    ast.num_tokens = token - *tokens;
    *tokens = token;
    return ast;
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
    ast.assignment.assignee = assignee;

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

Ast ast_general_id_parse(Token** tokens) {
    if (ast_interpret_as_function_declaration(*tokens)) {
        return ast_function_declaration_parse(tokens);
    }
    if (ast_interpret_as_variable_declaration(*tokens)) {
        return ast_variable_declaration_parse(tokens);
    }
    TokenType delimiters[] = {tt_end_statement};
    return ast_expression_parse(tokens, delimiters, arr_length(delimiters));
}

Ast ast_general_parse(Token** tokens) {
    switch ((*tokens)->type) {
        case tt_id: {
            return ast_general_id_parse(tokens);
            break;
        }
        case tt_return: {
            return ast_return_parse(tokens);
            break;
        }
        default: {
            break;
        }
    }
    massert(false, "should never happen");
    Ast error_ast = {0};
    return error_ast;
}
