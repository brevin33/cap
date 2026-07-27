#include "ast.h"

#include "cap.h"
#include "token.h"

Ast ast_scope(Tokens* tokens, Cap_File* file) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = tkns;
    ast.file = file;
    ast.kind = Ast_Kind_Scope;

    Token start_scope_token = tokens_get(tkns);
    ast_expect(start_scope_token, Token_Kind_Scope_Start);
    tokens_next(&tkns);
    log_msg_token("Parsed scope start", log_debug, start_scope_token, file);

    u32 ast_statements_capacity = 0;
    while (tokens_get(tkns).kind != Token_Kind_Scope_End) {
        Ast statement = ast_scoped_statement(&tkns, file);

        Token token = tokens_get(tkns);
        if (statement.kind != Ast_Kind_Invalid) {
            ptr_append(ast.data.scope.statements, ast.data.scope.ast_count, ast_statements_capacity, statement);
            log_msg_ast("Adding statement", log_debug, ast.data.scope.statements + ast.data.scope.ast_count - 1);
            if (token.kind != Token_Kind_End_Statement) {
                log_msg_token("Expected end statement", log_error, token, file);
            }
        }

        token = tokens_get(tkns);
        while (token.kind != Token_Kind_End_Statement && token.kind != Token_Kind_End_File) {
            tokens_next(&tkns);
            token = tokens_get(tkns);
        }
        while (token.kind == Token_Kind_End_Statement) {
            tokens_next(&tkns);
            token = tokens_get(tkns);
        }
        if (token.kind == Token_Kind_End_File) {
            log_msg_token("Reached end of file before scope end", log_error, start_scope_token, file);
            *tokens = tkns;
            return (Ast){0};
        }
    }
    Token end_scope_token = tokens_get(tkns);
    ast_expect(end_scope_token, Token_Kind_Scope_End);
    tokens_next(&tkns);

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

Ast ast_parameter(Tokens* tokens, Cap_File* file) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = tkns;
    ast.file = file;
    ast.kind = Ast_Kind_Parameter;

    Ast type = ast_expression(&tkns, file);
    if (type.kind == Ast_Kind_Invalid) return (Ast){0};

    Token token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Identifier);
    utf8 name = token.data;

    ast.data.parameter.type = alloc(sizeof(Ast));
    *ast.data.parameter.type = type;
    ast.data.parameter.name = name;

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

Ast ast_function_declaration(Tokens* tokens, Cap_File* file) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = tkns;
    ast.file = file;
    ast.kind = Ast_Kind_Function_Declaration;

    Ast* return_types = NULL;
    u32 return_types_count = 0;
    u32 return_types_capacity = 0;
    while (true) {
        Ast type = ast_expression(&tkns, file);
        if (type.kind == Ast_Kind_Invalid) return (Ast){0};
        ptr_append(return_types, return_types_count, return_types_capacity, type);
        Token token = tokens_get(tkns);
        if (token.kind == Token_Kind_Comma) {
            tokens_next(&tkns);
            continue;
        }
        break;
    }

    Token token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Identifier);
    utf8 name = token.data;
    tokens_next(&tkns);

    Ast parameter_list = ast_parameter_list(&tkns, file);
    if (parameter_list.kind == Ast_Kind_Invalid) return (Ast){0};

    Ast body = ast_scope(&tkns, file);
    if (body.kind == Ast_Kind_Invalid) return (Ast){0};

    ast.data.function_declaration.name = name;

    ast.data.function_declaration.return_types = return_types;
    ast.data.function_declaration.return_types_count = return_types_count;

    ast.data.function_declaration.parameter_list = alloc(sizeof(Ast));
    *ast.data.function_declaration.parameter_list = parameter_list;

    ast.data.function_declaration.body = alloc(sizeof(Ast));
    *ast.data.function_declaration.body = body;

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

Ast ast_parameter_list(Tokens* tokens, Cap_File* file) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = tkns;
    ast.file = file;
    ast.kind = Ast_Kind_Parameter_List;

    u32 ast_parameters_capacity = 0;

    Token token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Paren_Open);
    tokens_next(&tkns);

    token = tokens_get(tkns);
    if (token.kind != Token_Kind_Paren_Close) {
        while (true) {
            Ast parameter = ast_parameter(&tkns, file);
            if (parameter.kind == Ast_Kind_Invalid) return (Ast){0};
            ptr_append(ast.data.parameter_list.parameters, ast.data.parameter_list.parameters_count, ast_parameters_capacity, parameter);

            token = tokens_get(tkns);
            if (token.kind == Token_Kind_Paren_Close) break;
            else if (token.kind == Token_Kind_Comma) {
                tokens_next(&tkns);
            } else {
                log_msg_token("Expected comma or paren close", log_error, token, file);
                return (Ast){0};
            }
        }
    }
    token = tokens_get(tkns);
    assert(token.kind == Token_Kind_Paren_Close);
    tokens_next(&tkns);

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

Ast ast_build(Tokens* tokens, Cap_File* file) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = tkns;
    ast.file = file;
    ast.kind = Ast_Kind_Build;

    Token token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Build);
    tokens_next(&tkns);

    token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Identifier);
    ast.data.build.name = token.data;
    tokens_next(&tkns);

    Ast scope = ast_scope(&tkns, file);
    if (scope.kind == Ast_Kind_Invalid) return (Ast){0};
    ast.data.build.scope = alloc(sizeof(Ast));
    *ast.data.build.scope = scope;

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

Ast ast_return(Tokens* tokens, Cap_File* file) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = tkns;
    ast.file = file;
    ast.kind = Ast_Kind_Return;

    Token token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Return);
    tokens_next(&tkns);

    Ast* expressions = NULL;
    u32 expressions_count = 0;
    u32 expressions_capacity = 0;

    while (true) {
        Ast expression = ast_expression(&tkns, file);
        if (expression.kind == Ast_Kind_Invalid) return (Ast){0};
        ptr_append(expressions, expressions_count, expressions_capacity, expression);
        token = tokens_get(tkns);
        if (token.kind == Token_Kind_Comma) {
            tokens_next(&tkns);
            continue;
        }
        break;
    }
    ast.data.return_.values = expressions;
    ast.data.return_.values_count = expressions_count;

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

Ast ast_statement_starting_with_expression(Tokens* tokens, Cap_File* file) {
    u32 log_location = log_end_location();
    Ast function_declaration = ast_function_declaration(tokens, file);
    if (function_declaration.kind != Ast_Kind_Invalid) return function_declaration;
    log_clear_after(log_location);

    Tokens tkns = *tokens;

    Ast* lhs_values = NULL;
    u32 lhs_values_count = 0;
    u32 lhs_values_capacity = 0;

    Token token = tokens_get(tkns);
    while (true) {
        Ast lhs = ast_expression(&tkns, file);
        if (lhs.kind == Ast_Kind_Invalid) return (Ast){0};
        token = tokens_get(tkns);
        if (token.kind == Token_Kind_Identifier) {
            Ast variable_declaration = ast_variable_declaration(&tkns, file, &lhs);
            if (variable_declaration.kind == Ast_Kind_Invalid) return (Ast){0};
            lhs = variable_declaration;
        }
        ptr_append(lhs_values, lhs_values_count, lhs_values_capacity, lhs);
        token = tokens_get(tkns);
        if (token.kind != Token_Kind_Comma) break;
        tokens_next(&tkns);
    }
    token = tokens_get(tkns);
    if (token.kind == Token_Kind_Assign) {
        tokens_next(&tkns);
        Ast* rhs_values = NULL;
        u32 rhs_values_count = 0;
        u32 rhs_values_capacity = 0;
        while (true) {
            Ast rhs = ast_expression(&tkns, file);
            if (rhs.kind == Ast_Kind_Invalid) return (Ast){0};
            ptr_append(rhs_values, rhs_values_count, rhs_values_capacity, rhs);
            token = tokens_get(tkns);
            if (token.kind != Token_Kind_Comma) break;
            tokens_next(&tkns);
        }
        if (rhs_values_count != lhs_values_count && rhs_values_count != 1) {
            log_msg_token("Expected exactly one or as many values on rhs as lhs when assigning", log_error, token, file);
            return (Ast){0};
        }
        Ast assign = {0};
        assign.kind = Ast_Kind_Assign;
        assign.data.assign.lhs = lhs_values;
        assign.data.assign.lhs_count = lhs_values_count;
        assign.data.assign.rhs = rhs_values;
        assign.data.assign.rhs_count = rhs_values_count;

        Token* start = lhs_values[0].tokens.data;
        Token* end = rhs_values[rhs_values_count - 1].tokens.data;
        assign.tokens.data = start;
        assign.tokens.count = end - start;

        assign.file = file;
        *tokens = tkns;
        return assign;
    }
    if (lhs_values_count != 1) {
        log_msg_token("Expected exactly one value on lhs when not assigning", log_error, token, file);
        return (Ast){0};
    }

    // TODO: handel assignment and multiple values well
    Ast lhs_ast = lhs_values[0];
    *tokens = tkns;
    return lhs_ast;
}

Ast ast_scoped_statement(Tokens* tokens, Cap_File* file) {
    Token token = tokens_get(*tokens);
    switch (tokens_get(*tokens).kind) {
        case Token_Kind_Return: {
            return ast_return(tokens, file);
        }
        case Token_Kind_Identifier: {
            return ast_statement_starting_with_expression(tokens, file);
        }
        case Token_Kind_Build:
        case Token_Kind_Invalid:
        case Token_Kind_End_Statement:
        case Token_Kind_End_File:
        case Token_Kind_Int:
        case Token_Kind_Float:
        case Token_Kind_Scope_Start:
        case Token_Kind_Scope_End:
        case Token_Kind_Assign:
        case Token_Kind_Paren_Open:
        case Token_Kind_Paren_Close:
        case Token_Kind_Comma: {
            log_msg_token("Unexpected token when parsing scoped statement", log_error, token, file);
            return (Ast){0};
        }
    }
}

Ast ast_top_level_statement(Tokens* tokens, Cap_File* file) {
    Token token = tokens_get(*tokens);
    switch (token.kind) {
        case Token_Kind_Build: {
            return ast_build(tokens, file);
        }
        case Token_Kind_Identifier: {
            return ast_statement_starting_with_expression(tokens, file);
        }
        case Token_Kind_Return:
        case Token_Kind_Invalid:
        case Token_Kind_End_Statement:
        case Token_Kind_End_File:
        case Token_Kind_Int:
        case Token_Kind_Float:
        case Token_Kind_Scope_Start:
        case Token_Kind_Scope_End:
        case Token_Kind_Assign:
        case Token_Kind_Paren_Open:
        case Token_Kind_Paren_Close:
        case Token_Kind_Comma: {
            log_msg_token("Unexpected token when parsing top level statement", log_error, token, file);
            return (Ast){0};
        }
    }
}

utf8 ast_kind_to_string(Ast_Kind kind) {
    switch (kind) {
        case Ast_Kind_Invalid:
            return utf8_str("Ast_Kind_Invalid");
        case Ast_Kind_File:
            return utf8_str("Ast_Kind_File");
        case Ast_Kind_Scope:
            return utf8_str("Ast_Kind_Scope");
        case Ast_Kind_Return:
            return utf8_str("Ast_Kind_Return");
        case Ast_Kind_Int:
            return utf8_str("Ast_Kind_Int");
        case Ast_Kind_Float:
            return utf8_str("Ast_Kind_Float");
        case Ast_Kind_Variable:
            return utf8_str("Ast_Kind_Variable");
        case Ast_Kind_Variable_Declaration:
            return utf8_str("Ast_Kind_Variable_Declaration");
        case Ast_Kind_Binary_Operation:
            return utf8_str("Ast_Kind_Binary_Operation");
        case Ast_Kind_Parameter:
            return utf8_str("Ast_Kind_Parameter");
        case Ast_Kind_Parameter_List:
            return utf8_str("Ast_Kind_Parameter_List");
        case Ast_Kind_Function_Declaration:
            return utf8_str("Ast_Kind_Function_Declaration");
        case Ast_Kind_Call:
            return utf8_str("Ast_Kind_Call");
        case Ast_Kind_Argument_List:
            return utf8_str("Ast_Kind_Argument_List");
        case Ast_Kind_Build:
            return utf8_str("Ast_Kind_Build");
        case Ast_Kind_Intrinsic:
            return utf8_str("Ast_Kind_Intrinsic");
        case Ast_Kind_Assign:
            return utf8_str("Ast_Kind_Assign");
    }
}

Ast ast_int(Tokens* tokens, Cap_File* file) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = tkns;
    ast.file = file;
    ast.kind = Ast_Kind_Int;

    Token token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Int);
    tokens_next(&tkns);

    bool err = false;
    ast.data.int_.value = big_from_str(token.data.data, &err);
    if (err) {
        log_msg_token("Failed to parse into big int", log_error, token, file);
        return (Ast){0};
    }

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

Ast ast_float(Tokens* tokens, Cap_File* file) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = tkns;
    ast.file = file;
    ast.kind = Ast_Kind_Float;

    Token token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Float);
    tokens_next(&tkns);

    ast.data.float_.value = strtod(token.data.data, NULL);
    if (errno != ERANGE) {
        errno = 0;
        log_msg_token("Failed to parse into float", log_error, token, file);
        return (Ast){0};
    }

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

Ast ast_variable(Tokens* tokens, Cap_File* file) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = tkns;
    ast.file = file;
    ast.kind = Ast_Kind_Variable;

    Token token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Identifier);
    tokens_next(&tkns);

    ast.data.variable.name = token.data;

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

Ast ast_variable_declaration(Tokens* tokens, Cap_File* file, Ast* type) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = type->tokens;
    ast.file = file;
    ast.kind = Ast_Kind_Variable_Declaration;

    Token token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Identifier);
    tokens_next(&tkns);

    ast.data.variable_declaration.type = alloc(sizeof(Ast));
    *ast.data.variable_declaration.type = *type;
    ast.data.variable_declaration.name = token.data;

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

Ast ast_call(Tokens* tokens, Cap_File* file, Ast* callee) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = callee->tokens;
    ast.file = file;
    ast.kind = Ast_Kind_Call;

    ast.data.call.callee = alloc(sizeof(Ast));
    *ast.data.call.callee = *callee;

    Ast argument_list = ast_argument_list(&tkns, file);
    if (argument_list.kind == Ast_Kind_Invalid) return (Ast){0};

    ast.data.call.argument_list = alloc(sizeof(Ast));
    *ast.data.call.argument_list = argument_list;

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

Ast ast_argument_list(Tokens* tokens, Cap_File* file) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = tkns;
    ast.file = file;
    ast.kind = Ast_Kind_Argument_List;

    u32 ast_arguments_capacity = 0;

    Token token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Paren_Open);
    tokens_next(&tkns);

    token = tokens_get(tkns);
    if (token.kind != Token_Kind_Paren_Close) {
        while (true) {
            Ast argument = ast_expression(&tkns, file);
            if (argument.kind == Ast_Kind_Invalid) return (Ast){0};
            ptr_append(ast.data.argument_list.arguments, ast.data.argument_list.arguments_count, ast_arguments_capacity, argument);

            token = tokens_get(tkns);
            if (token.kind == Token_Kind_Paren_Close) break;
            else if (token.kind == Token_Kind_Comma) {
                tokens_next(&tkns);
            } else {
                log_msg_token("Expected comma or paren close", log_error, token, file);
                return (Ast){0};
            }
        }
    }
    assert(token.kind == Token_Kind_Paren_Close);

    if (ast.data.argument_list.arguments_count == 2) {
        Ast* first_argument = &ast.data.argument_list.arguments[0];
        Ast* second_argument = &ast.data.argument_list.arguments[1];
        debug_break();
    }

    tokens_next(&tkns);

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

Ast ast_binary_operation(Tokens* tokens, Cap_File* file, Ast* lhs, Ast* rhs, Token_Kind operator) {
    Tokens tkns = *tokens;

    Ast ast = {0};
    ast.tokens = lhs->tokens;
    ast.file = file;
    ast.kind = Ast_Kind_Binary_Operation;

    ast.data.binary_operation.lhs = alloc(sizeof(Ast));
    *ast.data.binary_operation.lhs = *lhs;

    ast.data.binary_operation.rhs = alloc(sizeof(Ast));
    *ast.data.binary_operation.rhs = *rhs;

    ast.data.binary_operation.operator = operator;

    ast.tokens.count = tkns.data - ast.tokens.data;
    *tokens = tkns;
    return ast;
}

static i32 _get_precedence(Token_Kind kind) {
    switch (kind) {
        case Token_Kind_Identifier:
        case Token_Kind_Invalid:
        case Token_Kind_End_Statement:
        case Token_Kind_End_File:
        case Token_Kind_Return:
        case Token_Kind_Build:
        case Token_Kind_Int:
        case Token_Kind_Float:
        case Token_Kind_Scope_Start:
        case Token_Kind_Scope_End:
        case Token_Kind_Assign:
        case Token_Kind_Paren_Open:
        case Token_Kind_Paren_Close:
        case Token_Kind_Comma: {
            return INT32_MIN;
        }
    }
}

static Ast ast_expression_value_paren_open(Tokens* tokens, Cap_File* file) {
    Tokens tkns = *tokens;

    Token token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Paren_Open);
    tokens_next(&tkns);

    Ast expression = ast_expression(&tkns, file);
    if (expression.kind == Ast_Kind_Invalid) return (Ast){0};

    token = tokens_get(tkns);
    ast_expect(token, Token_Kind_Paren_Close);
    tokens_next(&tkns);

    *tokens = tkns;
    return expression;
}

static Ast ast_expression_value(Tokens* tokens, Cap_File* file) {
    Token token = tokens_get(*tokens);
    switch (token.kind) {
        case Token_Kind_Identifier: {
            return ast_variable(tokens, file);
        }
        case Token_Kind_Int: {
            return ast_int(tokens, file);
        }
        case Token_Kind_Float: {
            return ast_float(tokens, file);
        }
        case Token_Kind_Paren_Open: {
            return ast_expression_value_paren_open(tokens, file);
        }
        case Token_Kind_Invalid:
        case Token_Kind_End_Statement:
        case Token_Kind_End_File:
        case Token_Kind_Return:
        case Token_Kind_Build:
        case Token_Kind_Scope_Start:
        case Token_Kind_Scope_End:
        case Token_Kind_Assign:
        case Token_Kind_Paren_Close:
        case Token_Kind_Comma: {
            log_msg_token("Unexpected token when parsing expression", log_error, token, file);
            return (Ast){0};
            break;
        }
    }
}

static Ast ast_expression_mono_operator(Tokens* tokens, Cap_File* file, Ast* lhs, bool* out_found_operator) {
    Tokens tkns = *tokens;
    Token token = tokens_get(tkns);
    *out_found_operator = true;
    switch (token.kind) {
        case Token_Kind_Paren_Open: {
            return ast_call(tokens, file, lhs);
        }
        case Token_Kind_Invalid:
        case Token_Kind_Identifier:
        case Token_Kind_End_Statement:
        case Token_Kind_End_File:
        case Token_Kind_Return:
        case Token_Kind_Build:
        case Token_Kind_Int:
        case Token_Kind_Float:
        case Token_Kind_Scope_Start:
        case Token_Kind_Scope_End:
        case Token_Kind_Paren_Close:
        case Token_Kind_Assign:
        case Token_Kind_Comma: {
            *out_found_operator = false;
            return (Ast){0};
        }
    }
}

static Ast _ast_expression(Tokens* tokens, Cap_File* file, i32 precedence) {
    Tokens tkns = *tokens;
    Ast lhs = ast_expression_value(&tkns, file);
    if (lhs.kind == Ast_Kind_Invalid) return (Ast){0};

    while (true) {
        bool found_mono_operator = false;
        Ast mono_operator = ast_expression_mono_operator(&tkns, file, &lhs, &found_mono_operator);
        if (found_mono_operator) {
            if (mono_operator.kind == Ast_Kind_Invalid) return (Ast){0};
            lhs = mono_operator;
            continue;
        }

        Token token = tokens_get(tkns);
        i32 operator_precedence = _get_precedence(token.kind);
        if (operator_precedence >= precedence) {
            *tokens = tkns;
            return lhs;
        }
        if (operator_precedence == INT32_MIN) {
            *tokens = tkns;
            return lhs;
        }

        Token_Kind operator = token.kind;
        tokens_next(&tkns);

        Ast rhs = _ast_expression(&tkns, file, operator_precedence);
        if (rhs.kind == Ast_Kind_Invalid) return (Ast){0};

        Ast binary_operation = ast_binary_operation(&tkns, file, &lhs, &rhs, operator);
        if (binary_operation.kind == Ast_Kind_Invalid) return (Ast){0};

        lhs = binary_operation;
    }
}

Ast ast_expression(Tokens* tokens, Cap_File* file) {
    Ast expr = _ast_expression(tokens, file, INT32_MAX);
    if (expr.kind == Ast_Kind_Invalid) return (Ast){0};
    return expr;
}

Ast ast_create_from_file(Cap_File* file) {
    Ast ast = {0};
    ast.tokens = file->tokens;
    ast.kind = Ast_Kind_File;
    ast.file = file;
    u32 ast_top_level_statements_capacity = 0;

    Tokens tokens = file->tokens;
    while (tokens_get(tokens).kind != Token_Kind_End_File) {
        Ast global_statement = ast_top_level_statement(&tokens, file);

        Token token = tokens_get(tokens);
        if (global_statement.kind != Ast_Kind_Invalid) {
            ptr_append(ast.data.file.top_level_statements, ast.data.file.ast_count, ast_top_level_statements_capacity, global_statement);
            log_msg_ast("Adding top level statement", log_debug, ast.data.file.top_level_statements + ast.data.file.ast_count - 1);
            if (token.kind != Token_Kind_End_Statement) {
                log_msg_token("Expected end statement", log_error, token, file);
            }
        }

        token = tokens_get(tokens);
        while (token.kind != Token_Kind_End_Statement && token.kind != Token_Kind_End_File) {
            tokens_next(&tokens);
            token = tokens_get(tokens);
        }
        while (token.kind == Token_Kind_End_Statement) {
            tokens_next(&tokens);
            token = tokens_get(tokens);
        }
        if (token.kind == Token_Kind_End_File) break;
    }

    return ast;
}

static void _ast_add_intrinsic(const char* name, Ast_Intrinsic intrinsic) {
    Ast* ast = alloc(sizeof(Ast));
    ast->kind = Ast_Kind_Intrinsic;
    ast->data.intrinsic = intrinsic;
    Scope_Variable* var = ast_add_variable_to_scope(&context.intrinsic_scope, utf8_str(name), ast);
    assert(var != NULL);
}

void ast_setup_intrinsics() {
    _ast_add_intrinsic("int_type", Ast_Intrinsic_Int_Type);
    _ast_add_intrinsic("uint_type", Ast_Intrinsic_Uint_Type);
    _ast_add_intrinsic("float_type", Ast_Intrinsic_Float_Type);
    _ast_add_intrinsic("compile_to_llvm_ir", Ast_Intrinsic_Compile_To_LLVM_IR);
    _ast_add_intrinsic("type", Ast_Intrinsic_Type);
    _ast_add_intrinsic("void", Ast_Intrinsic_Void);
    _ast_add_intrinsic("int_literal", Ast_Intrinsic_Int_Literal);
    _ast_add_intrinsic("float_literal", Ast_Intrinsic_Float_Literal);
    _ast_add_intrinsic("function", Ast_Intrinsic_Function);
}

bool ast_resolve_variables(Ast* ast, Scope* scope) {
    switch (ast->kind) {
        case Ast_Kind_Intrinsic:
        case Ast_Kind_Float:
        case Ast_Kind_Int: {
            return true;
        }
        case Ast_Kind_Scope: {
            ast->data.scope.scope = alloc(sizeof(Scope));
            Scope* new_scope = ast->data.scope.scope;
            new_scope->ast = ast;
            new_scope->parent = scope;
            for (u32 i = 0; i < ast->data.scope.ast_count; i++) {
                if (!ast_resolve_variables(ast->data.scope.statements + i, new_scope)) return false;
            }
            return true;
        }
        case Ast_Kind_Return: {
            for (u32 i = 0; i < ast->data.return_.values_count; i++) {
                Ast* value = ast->data.return_.values + i;
                if (!ast_resolve_variables(value, scope)) return false;
            }
            return true;
        }
        case Ast_Kind_Variable: {
            utf8 name = ast->data.variable.name;
            Scope_Variable* variable = ast_find_variable_in_scope(scope, name, ast);
            if (variable == NULL) return false;
            ast->data.variable.variable_declaration = variable->ast;
            return true;
        }
        case Ast_Kind_Variable_Declaration: {
            if (!ast_resolve_variables(ast->data.variable_declaration.type, scope)) return false;
            utf8 name = ast->data.variable_declaration.name;
            Scope_Variable* variable = ast_add_variable_to_scope(scope, name, ast);
            if (variable == NULL) return false;
            return true;
        }
        case Ast_Kind_Parameter_List: {
            ast->data.parameter_list.scope = alloc(sizeof(Scope));
            Scope* parameter_scope = ast->data.parameter_list.scope;
            parameter_scope->ast = ast;
            parameter_scope->parent = scope;
            Ast** semantic_parse_order = alloc(sizeof(Ast*) * ast->data.parameter_list.parameters_count);
            u32 semantic_parse_order_count = 0;
            bool changed = true;
            while (changed && semantic_parse_order_count < ast->data.parameter_list.parameters_count) {
                changed = false;
                for (u32 i = 0; i < ast->data.parameter_list.parameters_count; i++) {
                    Ast* parameter = ast->data.parameter_list.parameters + i;
                    bool already_added = false;
                    for (u32 j = 0; j < semantic_parse_order_count; j++) {
                        Ast* semantic_parse_order_parameter = semantic_parse_order[j];
                        if (parameter == semantic_parse_order_parameter) {
                            already_added = true;
                            break;
                        }
                    }
                    if (already_added) continue;

                    u32 log_location = log_end_location();
                    if (!ast_resolve_variables(parameter, parameter_scope)) {
                        log_clear_after(log_location);
                        continue;
                    }
                    changed = true;
                    semantic_parse_order[semantic_parse_order_count] = parameter;
                    semantic_parse_order_count++;
                }
            }
            if (semantic_parse_order_count != ast->data.parameter_list.parameters_count) {
                // run again to log errors
                for (u32 i = 0; i < ast->data.parameter_list.parameters_count; i++) {
                    Ast* parameter = ast->data.parameter_list.parameters + i;
                    bool already_added = false;
                    for (u32 j = 0; j < semantic_parse_order_count; j++) {
                        Ast* semantic_parse_order_parameter = semantic_parse_order[j];
                        if (parameter == semantic_parse_order_parameter) {
                            already_added = true;
                            break;
                        }
                    }
                    if (already_added) continue;
                    bool success = ast_resolve_variables(parameter, parameter_scope);
                    assert(!success);
                }
                return false;
            }
            ast->data.parameter_list.parameter_semantic_parse_order = semantic_parse_order;
            return true;
        }
        case Ast_Kind_Parameter: {
            if (!ast_resolve_variables(ast->data.parameter.type, scope)) return false;
            utf8 name = ast->data.parameter.name;
            Scope_Variable* variable = ast_add_variable_to_scope(scope, name, ast);
            if (variable == NULL) return false;
            return true;
        }
        case Ast_Kind_Binary_Operation: {
            Ast* lhs = ast->data.binary_operation.lhs;
            if (!ast_resolve_variables(lhs, scope)) return false;
            Ast* rhs = ast->data.binary_operation.rhs;
            if (!ast_resolve_variables(rhs, scope)) return false;
            return true;
        }
        case Ast_Kind_Function_Declaration: {
            if (!ast_resolve_variables(ast->data.function_declaration.parameter_list, scope)) return false;
            assert(ast->data.function_declaration.parameter_list->kind == Ast_Kind_Parameter_List);
            Scope* parameter_scope = ast->data.function_declaration.parameter_list->data.parameter_list.scope;
            for (u32 i = 0; i < ast->data.function_declaration.return_types_count; i++) {
                Ast* return_type = ast->data.function_declaration.return_types + i;
                if (!ast_resolve_variables(return_type, parameter_scope)) return false;
            }
            utf8 name = ast->data.function_declaration.name;
            Scope_Variable* variable = ast_add_variable_to_scope(scope, name, ast);
            if (variable == NULL) return false;
            if (!ast_resolve_variables(ast->data.function_declaration.body, parameter_scope)) return false;
            return true;
        }
        case Ast_Kind_Call: {
            Ast* callee = ast->data.call.callee;
            if (!ast_resolve_variables(callee, scope)) return false;
            Ast* arguments = ast->data.call.argument_list;
            if (!ast_resolve_variables(arguments, scope)) return false;
            return true;
        }
        case Ast_Kind_Argument_List: {
            for (u32 i = 0; i < ast->data.argument_list.arguments_count; i++) {
                Ast* argument = ast->data.argument_list.arguments + i;
                if (!ast_resolve_variables(argument, scope)) return false;
            }
            return true;
        }
        case Ast_Kind_Build: {
            return ast_resolve_variables(ast->data.build.scope, scope);
        }
        case Ast_Kind_File:
        case Ast_Kind_Invalid: {
            log_msg_ast("Unexpected ast kind", log_error, ast);
            return false;
        }
        case Ast_Kind_Assign: {
            Ast* lhs = ast->data.assign.lhs;
            Ast* rhs = ast->data.assign.rhs;
            if (!ast_resolve_variables(lhs, scope)) return false;
            if (!ast_resolve_variables(rhs, scope)) return false;
            return true;
        }
    }
}

bool ast_top_level_prototype_resolve_variables(Ast* ast, Scope* scope) {
    switch (ast->kind) {
        case Ast_Kind_Function_Declaration: {
            if (!ast_resolve_variables(ast->data.function_declaration.parameter_list, scope)) return false;
            Scope* parameter_scope = ast->data.function_declaration.parameter_list->data.parameter_list.scope;
            for (u32 i = 0; i < ast->data.function_declaration.return_types_count; i++) {
                Ast* return_type = ast->data.function_declaration.return_types + i;
                if (!ast_resolve_variables(return_type, parameter_scope)) return false;
            }
            utf8 name = ast->data.function_declaration.name;
            Scope_Variable* variable = ast_add_variable_to_scope(scope, name, ast);
            if (variable == NULL) return false;
            return true;
        }
        case Ast_Kind_Build: {
            return true;
        }
        default: {
            return ast_resolve_variables(ast, scope);
        }
    }
}

bool ast_top_level_implement_resolve_variables(Ast* ast, Scope* scope) {
    switch (ast->kind) {
        case Ast_Kind_Function_Declaration: {
            Scope* parameter_scope = ast->data.function_declaration.parameter_list->data.parameter_list.scope;
            if (!ast_resolve_variables(ast->data.function_declaration.body, parameter_scope)) return false;
            return true;
        }
        case Ast_Kind_Build: {
            return ast_resolve_variables(ast->data.build.scope, scope);
        }
        default: {
            return true;
        }
    }
}

static Scope_Variable* _ast_find_variable_in_scope(Scope* scope, utf8 name) {
    for (u32 i = 0; i < scope->variables_count; i++) {
        Scope_Variable* variable = scope->variables + i;
        if (utf8_equal(variable->name, name)) {
            return variable;
        }
    }
    if (scope->parent != NULL) return _ast_find_variable_in_scope(scope->parent, name);
    return NULL;
}

Scope_Variable* ast_find_variable_in_scope(Scope* scope, utf8 name, Ast* ast) {
    Scope_Variable* variable = _ast_find_variable_in_scope(scope, name);
    if (variable == NULL) {
        log_msg_ast("Variable not found", log_error, ast);
    }
    return variable;
}

Scope_Variable* ast_add_variable_to_scope(Scope* scope, utf8 name, Ast* ast) {
    for (u32 i = 0; i < scope->variables_count; i++) {
        Scope_Variable* variable = scope->variables + i;
        if (utf8_equal(variable->name, name)) {
            log_msg_ast("Variable already exists", log_error, ast);
            log_msg_ast("Exists here", log_info, variable->ast);
            return NULL;
        }
    }
    Scope_Variable variable = {0};
    variable.name = name;
    variable.ast = ast;
    ptr_append(scope->variables, scope->variables_count, scope->variables_capacity, variable);
    return scope->variables + scope->variables_count - 1;
}
