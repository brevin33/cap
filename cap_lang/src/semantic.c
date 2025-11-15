#include "cap.h"
#include "cap/ast.h"
#include "cap/lists.h"

void sem_templated_function_implement(Templated_Function* templated_function) {
    Function* function = templated_function->function;
    massert(!function->is_prototype, "should not be a prototype");

    templated_function->body.parent = NULL;
    templated_function->body.ast = function->ast->function_declaration.body;

    for (u32 i = 0; i < function->parameters.count; i++) {
        Function_Parameter* parameter = Function_Parameter_List_get(&function->parameters, i);
        Variable variable;
        Ast* parameter_ast = parameter->ast;
        variable.type = sem_type_parse(parameter_ast->declaration_parameter.type, &templated_function->allocator_id_counter);
        variable.name = parameter->name;
        variable.ast = parameter->ast;
        sem_scope_add_variable(&templated_function->body, &variable);
    }

    sem_scope_implement(&templated_function->body, templated_function);
}

Function* sem_function_prototype(Ast* ast) {
    massert(ast->kind == ast_function_declaration, "should have found a function declaration");
    Function* function = alloc(sizeof(Function));
    function->ast = ast;
    u32 fake_allocator_id_counter = 0;
    function->return_type = sem_type_parse(ast->function_declaration.return_type, &fake_allocator_id_counter);
    function->name = ast->function_declaration.name;

    for (u32 i = 0; i < ast->function_declaration.parameter_list->declaration_parameter_list.parameters.count; i++) {
        Ast* parameter_ast = &ast->function_declaration.parameter_list->declaration_parameter_list.parameters.data[i];
        Function_Parameter parameter;
        parameter.ast = parameter_ast;
        parameter.type = sem_type_parse(parameter_ast->declaration_parameter.type, &fake_allocator_id_counter);
        parameter.name = parameter_ast->declaration_parameter.name;
        Function_Parameter_List_add(&function->parameters, &parameter);
    }
    function->is_prototype = ast->function_declaration.body == NULL;

    if (!function->is_template_function) {
        Templated_Function* templated_function = alloc(sizeof(Templated_Function));
        templated_function->function = function;
        Templated_Function_Ptr_List_add(&function->templated_functions, &templated_function);
    }

    return function;
}

void sem_scope_implement(Scope* scope, Templated_Function* templated_function) {
    Ast* ast = scope->ast;
    massert(ast->kind == ast_body, "should have found a body");
    for (u32 i = 0; i < ast->body.statements.count; i++) {
        Ast* statement_ast = &ast->body.statements.data[i];
        Statement statement = sem_statement_parse(statement_ast, scope, templated_function);
        if (statement.type == statement_invalid) continue;
        Statement_List_add(&scope->statements, &statement);
    }
}

void sem_default_setup_types() {
    Type_Base* invalid_type = alloc(sizeof(Type_Base));
    invalid_type->name = alloc(sizeof("invalid"));
    memcpy(invalid_type->name, "invalid", sizeof("invalid"));
    invalid_type->kind = type_invalid;
    Type_Base_Ptr_List_add(&cap_context.types, &invalid_type);

    Type_Base* const_int_type = alloc(sizeof(Type_Base));
    const_int_type->name = alloc(sizeof("$const_int$"));
    memcpy(const_int_type->name, "const_int", sizeof("const_int"));
    const_int_type->kind = type_const_int;
    Type_Base_Ptr_List_add(&cap_context.types, &const_int_type);

    Type_Base* const_float_type = alloc(sizeof(Type_Base));
    const_float_type->name = alloc(sizeof("$const_float$"));
    memcpy(const_float_type->name, "const_float", sizeof("const_float"));
    const_float_type->kind = type_const_float;
    Type_Base_Ptr_List_add(&cap_context.types, &const_float_type);
}

Type_Base* add_number_type_base(u32 number_bit_size, Type_Kind kind) {
    Type_Base* type_base = alloc(sizeof(Type_Base));
    type_base->name = alloc(sizeof("number"));
    memcpy(type_base->name, "number", sizeof("number"));
    type_base->kind = kind;
    type_base->number_bit_size = number_bit_size;
    Type_Base_Ptr_List_add(&cap_context.types, &type_base);
    return type_base;
}

Type_Base* sem_find_type_base(const char* name) {
    for (u32 i = 0; i < cap_context.types.count; i++) {
        Type_Base* type_base = cap_context.types.data[i];
        if (strcmp(type_base->name, name) == 0) {
            return type_base;
        }
    }
    int number_size;
    if (sscanf_s(name, "i%d", &number_size) == 1) {
        return add_number_type_base(number_size, type_int);
    }
    if (sscanf_s(name, "f%d", &number_size) == 1) {
        return add_number_type_base(number_size, type_float);
    }
    if (sscanf_s(name, "u%d", &number_size) == 1) {
        return add_number_type_base(number_size, type_uint);
    }
    return NULL;
}

Type sem_type_parse(Ast* ast, u32* allocator_id_counter) {
    massert(ast->kind == ast_type, "should have found a type");
    Type type = {0};

    type.base = sem_find_type_base(ast->type.name);
    if (type.base == NULL) {
        log_error_ast(ast, "could not find type %s", ast->type.name);
        type.base = cap_context.types.data[0];  // first index is invalid
    }

    type.base_allocator_id = *allocator_id_counter;
    *allocator_id_counter += 1;

    type.ptr_count = 0;
    type.is_ref = false;
    return type;
}

Type sem_type_get_const_int(u32* allocator_id_counter) {
    Type type = {0};
    type.base = sem_find_type_base("const_int");
    type.base_allocator_id = *allocator_id_counter;
    *allocator_id_counter += 1;
    type.ptr_count = 0;
    type.is_ref = false;
    return type;
}

Type sem_type_get_const_float(u32* allocator_id_counter) {
    Type type = {0};
    type.base = sem_find_type_base("const_float");
    type.base_allocator_id = *allocator_id_counter;
    *allocator_id_counter += 1;
    type.ptr_count = 0;
    type.is_ref = false;
    return type;
}

Variable* sem_scope_get_variable(Scope* scope, char* name) {
    for (u32 i = 0; i < scope->variables.count; i++) {
        Variable* variable = *Variable_Ptr_List_get(&scope->variables, i);
        if (strcmp(variable->name, name) == 0) {
            return variable;
        }
    }
    if (scope->parent != NULL) {
        return sem_scope_get_variable(scope->parent, name);
    }
    return NULL;
}

Variable* sem_scope_add_variable(Scope* scope, Variable* variable) {
    for (u32 i = 0; i < scope->variables.count; i++) {
        Variable* existing_variable = *Variable_Ptr_List_get(&scope->variables, i);
        if (strcmp(existing_variable->name, variable->name) == 0) {
            log_error_ast(variable->ast, "variable %s already declared", variable->name);
            return NULL;
        }
    }
    Variable* ptr = alloc(sizeof(Variable));
    *ptr = *variable;
    Variable_Ptr_List_add(&scope->variables, &ptr);
    return ptr;
}

Statement sem_statement_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    switch (ast->kind) {
        case ast_assignment:
            return sem_statement_assignment_parse(ast, scope, templated_function);
        case ast_return:
            return sem_statement_return_parse(ast, scope, templated_function);
        case ast_expression:
            return sem_statement_expression_parse(ast, scope, templated_function);
        case ast_variable_declaration:
            return sem_statement_variable_declaration_parse(ast, scope, templated_function);
        case ast_int:
        case ast_float:
        case ast_variable:
        case ast_invalid:
        case ast_top_level:
        case ast_function_declaration:
        case ast_declaration_parameter_list:
        case ast_declaration_parameter:
        case ast_body:
        case ast_type: {
            log_error_ast(ast, "not a valid statement for the scope");
            Statement statement = {0};
            return statement;
        }
    }
}

Statement sem_statement_assignment_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_assignment, "should have found an assignment");
    Statement statement = {0};
    statement.ast = ast;
    statement.type = statement_assignment;
    statement.assignment.assignee = sem_expression_parse(ast->assignment.assignee, scope, templated_function);
    statement.assignment.value = sem_expression_parse(ast->assignment.value, scope, templated_function);
    return statement;
}

Statement sem_statement_return_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_return, "should have found a return");
    Statement statement = {0};
    statement.ast = ast;
    statement.type = statement_return;
    statement.return_.value = sem_expression_parse(ast->return_.value, scope, templated_function);
    return statement;
}

Statement sem_statement_expression_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_expression, "should have found an expression");
    Statement statement = {0};
    statement.ast = ast;
    statement.type = statement_expression;
    statement.expression.value = sem_expression_parse(ast->expression.value, scope, templated_function);
    return statement;
}

Statement sem_statement_variable_declaration_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_variable_declaration, "should have found a variable declaration");
    Statement statement = {0};
    statement.ast = ast;
    statement.type = statement_variable_declaration;

    Variable variable;
    variable.type = sem_type_parse(ast->variable_declaration.type, &templated_function->allocator_id_counter);
    if (!sem_set_allocator(templated_function, variable.type.base_allocator_id, &STACK_ALLOCATOR)) {
        log_error_ast(ast, "could not set stack allocator");
        Statement err = {0};
        return err;
    }
    variable.name = ast->variable_declaration.name;
    variable.ast = ast;

    Variable* in_scope = sem_scope_add_variable(scope, &variable);
    statement.variable_declaration.variable = in_scope;

    if (ast->variable_declaration.assignment != NULL) {
        Statement assignment = sem_statement_assignment_parse(ast->variable_declaration.assignment, scope, templated_function);
        if (assignment.type == statement_invalid) return assignment;
        statement.variable_declaration.assignment = alloc(sizeof(Statement));
        *statement.variable_declaration.assignment = assignment;
    } else {
        statement.variable_declaration.assignment = NULL;
    }

    return statement;
}

Expression sem_expression_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_expression, "should have found an expression value");
    Ast* value_ast = ast->expression.value;
    switch (value_ast->kind) {
        case ast_int:
            return sem_expression_int_parse(value_ast, scope, templated_function);
        case ast_float:
            return sem_expression_float_parse(value_ast, scope, templated_function);
        case ast_variable:
            return sem_expression_variable_parse(value_ast, scope, templated_function);
        case ast_variable_declaration:
        case ast_invalid:
        case ast_top_level:
        case ast_function_declaration:
        case ast_declaration_parameter_list:
        case ast_declaration_parameter:
        case ast_body:
        case ast_type:
        case ast_assignment:
        case ast_return:
        case ast_expression: {
            massert(false, "should never happen");
            Expression expression = {0};
            return expression;
        }
    }
}

Expression sem_expression_variable_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    Variable* variable = sem_scope_get_variable(scope, ast->variable.name);
    if (variable == NULL) {
        log_error_ast(ast, "could not find variable %s", ast->variable.name);
        Expression expression = {0};
        return expression;
    }
    Expression expression = {0};
    expression.ast = ast;
    expression.type = variable->type;
    expression.kind = expression_variable;
    expression.variable.variable = variable;
    return expression;
}

Expression sem_expression_int_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    char* value = ast->int_.value;
    Expression expression = {0};
    expression.ast = ast;
    expression.type = sem_type_get_const_int(&templated_function->allocator_id_counter);
    expression.kind = expression_int;
    expression.int_.value = value;
    return expression;
}

Expression sem_expression_float_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    double value = ast->float_.value;
    Expression expression = {0};
    expression.ast = ast;
    expression.type = sem_type_get_const_float(&templated_function->allocator_id_counter);
    expression.kind = expression_float;
    expression.float_.value = value;
    return expression;
}

static void sem_handel_graph_additions_if_needed(Templated_Function* templated_function, u32 allocator_id) {
    u32 number_of_allocators = templated_function->allocators.count;
    if (number_of_allocators <= allocator_id) {
        for (u32 i = number_of_allocators; i <= allocator_id; i++) {
            Allocator base_allocator = {0};
            Allocator_List_add(&templated_function->allocators, &base_allocator);

            u32_List base_connection = {0};
            u32_List_List_add(&templated_function->allocator_id_connections, &base_connection);
        }
    }
}

bool sem_allocator_are_the_same(Allocator* allocator1, Allocator* allocator2) {
    return allocator1->variable == allocator2->variable;
}

bool sem_added_allocator_id_connection(Templated_Function* templated_function, u32 allocator_id1, u32 allocator_id2) {
    sem_handel_graph_additions_if_needed(templated_function, allocator_id1);

    Allocator* allocator1 = sem_allocator_get(templated_function, allocator_id1);
    Allocator* allocator2 = sem_allocator_get(templated_function, allocator_id2);

    if (!sem_allocator_are_the_same(allocator1, allocator2)) {
        if (sem_allocator_are_the_same(allocator1, &UNDEFINED_ALLOCATOR)) {
            sem_set_allocator(templated_function, allocator_id1, allocator2);
        } else if (sem_allocator_are_the_same(allocator2, &UNDEFINED_ALLOCATOR)) {
            sem_set_allocator(templated_function, allocator_id2, allocator1);
        } else {
            return false;
        }
    }

    u32_List* connections1 = &templated_function->allocator_id_connections.data[allocator_id1];
    u32_List_add(connections1, &allocator_id2);

    u32_List* connections2 = &templated_function->allocator_id_connections.data[allocator_id2];
    u32_List_add(connections2, &allocator_id1);

    return true;
}

bool sem_set_allocator(Templated_Function* templated_function, u32 allocator_id, Allocator* allocator) {
    sem_handel_graph_additions_if_needed(templated_function, allocator_id);

    Allocator* existing_allocator = sem_allocator_get(templated_function, allocator_id);
    if (sem_allocator_are_the_same(existing_allocator, allocator)) return true;
    if (!sem_allocator_are_the_same(existing_allocator, &UNDEFINED_ALLOCATOR)) return false;

    *Allocator_List_get(&templated_function->allocators, allocator_id) = *allocator;
    u32_List connections = templated_function->allocator_id_connections.data[allocator_id];
    for (u32 i = 0; i < connections.count; i++) {
        u32 connection = *u32_List_get(&connections, i);
        Allocator* connection_allocator = sem_allocator_get(templated_function, connection);
        if (sem_allocator_are_the_same(connection_allocator, allocator)) continue;
        bool res = sem_set_allocator(templated_function, connection, allocator);
        massert(res, "should have set allocator");
    }

    return true;
}

Allocator* sem_allocator_get(Templated_Function* templated_function, u32 allocator_id) {
    sem_handel_graph_additions_if_needed(templated_function, allocator_id);
    return &templated_function->allocators.data[allocator_id];
}
