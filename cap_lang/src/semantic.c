#include "cap/semantic.h"

#include "cap.h"
#include "cap/ast.h"
#include "cap/lists.h"

Program* sem_program_parse(Ast* ast) {
    massert(ast->kind == ast_program, "should have found a program");
    Program* program = alloc(sizeof(Program));
    program->ast = ast;
    program->name = ast->program.name;

    program->body.ast = ast;
    u32 fake_allocator_id_counter = 0;
    program->body.return_type = sem_get_int_type(32, &fake_allocator_id_counter);
    program->body.name = ast->program.name;
    program->body.is_program = true;

    Templated_Function* templated_function = alloc(sizeof(Templated_Function));
    templated_function->function = &program->body;
    templated_function->return_type = sem_get_int_type(32, &templated_function->allocator_id_counter);
    sem_mark_type_as_function_value(templated_function, &templated_function->return_type, true);
    for (u32 i = 0; i < templated_function->function->parameters.count; i++) {
        Function_Parameter* parameter = Function_Parameter_List_get(&templated_function->function->parameters, i);
        Variable variable;
        Ast* parameter_ast = parameter->ast;
        variable.type = sem_type_parse(parameter_ast->declaration_parameter.type, &templated_function->allocator_connection_map, &templated_function->parameter_scope, &templated_function->allocator_id_counter);
        massert(variable.type.base->kind != type_invalid, "should have found a valid type");
        sem_mark_type_as_function_value(templated_function, &variable.type, false);
        variable.name = parameter->name;
        variable.ast = parameter->ast;
        sem_scope_add_variable(&templated_function->parameter_scope, &variable, templated_function);
    }
    templated_function->body.ast = ast->program.body;
    templated_function->body.parent = &templated_function->parameter_scope;

    Templated_Function_Ptr_List_add(&program->body.templated_functions, &templated_function);

    sem_templated_function_implement(program->body.templated_functions.data[0], ast->program.body);

    return program;
}

void sem_templated_function_implement(Templated_Function* templated_function, Ast* scope_ast) {
    if (templated_function->body.ast == NULL) return;
    sem_scope_implement(&templated_function->body, templated_function);
}

Function* sem_function_prototype(Ast* ast) {
    massert(ast->kind == ast_function_declaration, "should have found a function declaration");
    Function* function = alloc(sizeof(Function));
    Function_Ptr_List_add(&cap_context.functions, &function);
    function->ast = ast;
    u32 fake_allocator_id_counter = 0;
    function->return_type = sem_type_parse(ast->function_declaration.return_type, NULL, NULL, &fake_allocator_id_counter);
    if (function->return_type.base->kind == type_invalid) return NULL;
    function->name = ast->function_declaration.name;

    for (u32 i = 0; i < ast->function_declaration.parameter_list->declaration_parameter_list.parameters.count; i++) {
        Ast* parameter_ast = &ast->function_declaration.parameter_list->declaration_parameter_list.parameters.data[i];
        Function_Parameter parameter;
        parameter.ast = parameter_ast;
        parameter.type = sem_type_parse(parameter_ast->declaration_parameter.type, NULL, NULL, &fake_allocator_id_counter);
        if (parameter.type.base->kind == type_invalid) continue;
        parameter.name = parameter_ast->declaration_parameter.name;
        Function_Parameter_List_add(&function->parameters, &parameter);
    }
    function->is_prototype = ast->function_declaration.body == NULL;

    if (!function->is_template_function) {
        Templated_Function* templated_function = alloc(sizeof(Templated_Function));
        templated_function->function = function;
        for (u32 i = 0; i < templated_function->function->parameters.count; i++) {
            Function_Parameter* parameter = Function_Parameter_List_get(&templated_function->function->parameters, i);
            Variable variable;
            Ast* parameter_ast = parameter->ast;
            variable.type = sem_type_parse(parameter_ast->declaration_parameter.type, &templated_function->allocator_connection_map, &templated_function->parameter_scope, &templated_function->allocator_id_counter);
            massert(variable.type.base->kind != type_invalid, "should have found a valid type");
            sem_mark_type_as_function_value(templated_function, &variable.type, false);
            variable.name = parameter->name;
            variable.ast = parameter->ast;
            sem_scope_add_variable(&templated_function->parameter_scope, &variable, templated_function);
        }
        templated_function->body.ast = ast->function_declaration.body;
        templated_function->body.parent = &templated_function->parameter_scope;
        templated_function->return_type = sem_type_parse(templated_function->function->ast->function_declaration.return_type, &templated_function->allocator_connection_map, &templated_function->parameter_scope, &templated_function->allocator_id_counter);
        massert(templated_function->return_type.base->kind != type_invalid, "should have found a valid return type");
        sem_mark_type_as_function_value(templated_function, &templated_function->return_type, true);
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

    Type_Base* void_type = alloc(sizeof(Type_Base));
    void_type->name = alloc(sizeof("void"));
    memcpy(void_type->name, "void", sizeof("void"));
    void_type->kind = type_void;
    Type_Base_Ptr_List_add(&cap_context.types, &void_type);

    Type_Base* _type_type = alloc(sizeof(Type_Base));
    _type_type->name = alloc(sizeof("type"));
    memcpy(_type_type->name, "type", sizeof("type"));
    _type_type->kind = type_type;
    Type_Base_Ptr_List_add(&cap_context.types, &_type_type);
}

Type_Base* add_number_type_base(u32 number_bit_size, Type_Kind kind) {
    Type_Base* type_base = alloc(sizeof(Type_Base));

    u64 number_of_digits = get_number_of_digits(number_bit_size);
    char* type_name = alloc(number_of_digits + 2);
    if (kind == type_int) {
        sprintf(type_name, "i%u", number_bit_size);
    } else if (kind == type_float) {
        sprintf(type_name, "f%u", number_bit_size);
    } else if (kind == type_uint) {
        sprintf(type_name, "u%u", number_bit_size);
    } else {
        massert(false, "should have found a valid type");
    }
    type_base->name = type_name;

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

Allocator sem_allocator_parse(Ast* ast, Scope* scope) {
    massert(ast->kind == ast_allocator, "should have found an allocator");
    Variable* variable = sem_scope_get_variable(scope, ast->allocator.variable_name);
    if (variable == NULL) {
        if (strcmp(ast->allocator.variable_name, "stack") == 0) {
            return STACK_ALLOCATOR;
        }
        log_error_ast(ast, "could not find variable %s", ast->allocator.variable_name);
        return INVALID_ALLOCATOR;
    }
    if (ast->allocator.field_name != NULL) {
        massert(false, "not implemented");
    }
    return (Allocator){.variable = variable};
}

Struct_Field_Allocator_Info sem_struct_field_allocator_info_parse(Ast* ast, Struct_Field_List* other_feilds) {
    massert(ast->kind == ast_allocator, "should have found an allocator");
    char* name = ast->allocator.variable_name;
    Struct_Field* other = NULL;
    for (u32 i = 0; i < other_feilds->count; i++) {
        Struct_Field* field = &other_feilds->data[i];
        if (strcmp(field->name, name) == 0) {
            other = field;
            break;
        }
    }
    if (other == NULL) {
        massert(false, "not implemented");
    }
    if (ast->allocator.field_name != NULL) {
        massert(false, "not implemented");
    }

    Struct_Field_Allocator_Info info = {0};
    info.field = other;
    info.is_field = true;
    return info;
}

Struct_Field sem_struct_field_parse(Ast* ast, Struct_Field_List* other_feilds) {
    massert(ast->kind == ast_struct_field, "should have found a struct field");
    Struct_Field field_ = {0};
    Ast* type_ast = ast->struct_field.type;

    u32 fake_allocator_id_counter = 0;
    Type type = sem_type_parse(type_ast, NULL, NULL, &fake_allocator_id_counter);
    if (type_ast->type.base_allocator->kind == ast_allocator) {
        Ast* allocator_ast = type_ast->type.base_allocator;
        Struct_Field_Allocator_Info info = sem_struct_field_allocator_info_parse(allocator_ast, other_feilds);
        field_.base_allocator_info = info;
    } else {
        Struct_Field_Allocator_Info info = {0};
        field_.base_allocator_info = info;
    }

    for (u32 i = 0; i < type.ptr_count; i++) {
        Ast* ptr_allocator_ast = &type_ast->type.ptr_allocators.data[i];
        if (ptr_allocator_ast->kind == ast_allocator) {
            Struct_Field_Allocator_Info info = sem_struct_field_allocator_info_parse(ptr_allocator_ast, other_feilds);
            Struct_Field_Allocator_Info_List_add(&field_.ptr_allocator_infos, &info);
        } else {
            Struct_Field_Allocator_Info info = {0};
            Struct_Field_Allocator_Info_List_add(&field_.ptr_allocator_infos, &info);
        }
    }

    field_.type = type;
    field_.name = ast->struct_field.name;
    return field_;
}

void sem_add_struct(Ast* ast) {
    massert(ast->kind == ast_struct, "should have found a struct");
    Type_Base* type_base = alloc(sizeof(Type_Base));
    type_base->kind = type_struct;
    type_base->name = ast->struct_.name;

    Type_Base_Struct* type_struct = &type_base->struct_;
    Ast* body = ast->struct_.body;
    for (u32 i = 0; i < body->struct_body.fields.count; i++) {
        Ast* field = &body->struct_body.fields.data[i];
        Struct_Field field_ = sem_struct_field_parse(field, &type_base->struct_.fields);
        Struct_Field_List_add(&type_struct->fields, &field_);
    }

    Type_Base_Ptr_List_add(&cap_context.types, &type_base);
}

Type sem_type_parse(Ast* ast, Allocator_Connection_Map* map, Scope* scope, u32* allocator_id_counter) {
    massert(ast->kind == ast_type, "should have found a type");
    Type type = {0};

    type.base = sem_find_type_base(ast->type.name);
    if (type.base == NULL) {
        log_error_ast(ast, "could not find type %s", ast->type.name);
        type.base = cap_context.types.data[0];  // first index is invalid
    }

    type.base_allocator_id = *allocator_id_counter;
    *allocator_id_counter += 1;

    if (map != NULL) {
        massert(scope != NULL, "should have found a scope");
        Ast* allocator = ast->type.base_allocator;
        if (allocator->kind != ast_invalid) {
            Allocator new_allocator = sem_allocator_parse(allocator, scope);
            if (!sem_allocator_are_the_same(&new_allocator, &INVALID_ALLOCATOR))
                sem_set_allocator(map, type.base_allocator_id, &new_allocator);
        }
    }

    type.ptr_count = ast->type.ptr_count;
    if (type.ptr_count > 0) {
        type.ptr_allocator_ids = alloc(sizeof(u32) * type.ptr_count);
        for (u32 i = 0; i < type.ptr_count; i++) {
            type.ptr_allocator_ids[i] = *allocator_id_counter;
            *allocator_id_counter += 1;

            if (map != NULL) {
                massert(scope != NULL, "should have found a scope");
                Ast* allocator = &ast->type.ptr_allocators.data[i];
                if (allocator->kind == ast_invalid) continue;
                Allocator new_allocator = sem_allocator_parse(allocator, scope);
                if (sem_allocator_are_the_same(&new_allocator, &INVALID_ALLOCATOR)) continue;
                sem_set_allocator(map, type.ptr_allocator_ids[i], &new_allocator);
            }
        }
    }

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

Type sem_type_get_void(u32* allocator_id_counter) {
    Type type = {0};
    type.base = sem_find_type_base("void");
    type.base_allocator_id = *allocator_id_counter;
    *allocator_id_counter += 1;
    type.ptr_count = 0;
    type.is_ref = false;
    return type;
}

Type sem_type_get_type(u32* allocator_id_counter) {
    Type type = {0};
    type.base = sem_find_type_base("type");
    type.base_allocator_id = *allocator_id_counter;
    *allocator_id_counter += 1;
    type.ptr_count = 0;
    type.is_ref = false;
    return type;
}

Type sem_get_int_type(u32 bit_size, u32* allocator_id_counter) {
    Type type = {0};
    u64 number_of_digits = get_number_of_digits(bit_size);
    char* type_name = alloc(number_of_digits + 2);
    sprintf(type_name, "i%u", bit_size);
    type.base = sem_find_type_base(type_name);
    type.base_allocator_id = *allocator_id_counter;
    *allocator_id_counter += 1;
    type.ptr_count = 0;
    type.is_ref = false;
    return type;
}

Type sem_get_float_type(u32 number_bit_size, u32* allocator_id_counter) {
    Type type = {0};
    u64 number_of_digits = get_number_of_digits(number_bit_size);
    char* type_name = alloc(number_of_digits + 2);
    sprintf(type_name, "f%u", number_bit_size);
    type.base = sem_find_type_base(type_name);
    type.base_allocator_id = *allocator_id_counter;
    *allocator_id_counter += 1;
    type.ptr_count = 0;
    type.is_ref = false;
    return type;
}

Type sem_get_uint_type(u32 bit_size, u32* allocator_id_counter) {
    Type type = {0};
    u64 number_of_digits = get_number_of_digits(bit_size);
    char* type_name = alloc(number_of_digits + 2);
    sprintf(type_name, "u%u", bit_size);
    type.base = sem_find_type_base(type_name);
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

Variable* sem_scope_add_variable(Scope* scope, Variable* variable, Templated_Function* templated_function) {
    if (!variable->type.is_ref) {
        variable->type.ref_allocator_id = templated_function->allocator_id_counter;
        templated_function->allocator_id_counter += 1;
        sem_set_allocator(&templated_function->allocator_connection_map, variable->type.ref_allocator_id, &STACK_ALLOCATOR);
        variable->type.is_ref = true;
    }

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
        case ast_struct:
        case ast_struct_body:
        case ast_struct_field:
        case ast_get:
        case ast_ptr:
        case ast_allocator:
        case ast_add:
        case ast_sub:
        case ast_mul:
        case ast_div:
        case ast_mod:
        case ast_bit_and:
        case ast_bit_or:
        case ast_bit_xor:
        case ast_bit_not:
        case ast_bit_shl:
        case ast_bit_shr:
        case ast_and:
        case ast_or:
        case ast_equals_equals:
        case ast_not_equals:
        case ast_less_than:
        case ast_greater_than:
        case ast_less_than_equals:
        case ast_greater_than_equals:
        case ast_value_access:
        case ast_function_call:
        case ast_function_call_parameter:
        case ast_alloc:
        case ast_program:
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
    Statement err = {0};
    Statement statement = {0};
    statement.ast = ast;
    statement.type = statement_assignment;

    statement.assignment.assignee = sem_expression_parse(ast->assignment.assignee, scope, templated_function);
    if (statement.assignment.assignee.kind == expression_invalid) return err;
    if (!statement.assignment.assignee.type.is_ref) {
        log_error_ast(ast->assignment.assignee, "cannot assign to a non reference value");
        return err;
    }
    Type deref_type = sem_dereference_type(&statement.assignment.assignee.type);

    statement.assignment.value = sem_expression_parse(ast->assignment.value, scope, templated_function);
    if (statement.assignment.value.kind == expression_invalid) return err;
    statement.assignment.value = sem_expression_implicit_cast(&statement.assignment.value, &deref_type, templated_function);
    if (statement.assignment.value.kind == expression_invalid) return err;

    return statement;
}

Statement sem_statement_return_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_return, "should have found a return");
    Statement err = {0};
    Statement statement = {0};
    statement.ast = ast;
    statement.type = statement_return;
    if (ast->return_.value == NULL) return statement;
    statement.return_.value = sem_expression_parse(ast->return_.value, scope, templated_function);
    if (statement.return_.value.kind == expression_invalid) return err;
    statement.return_.value = sem_expression_implicit_cast(&statement.return_.value, &templated_function->return_type, templated_function);
    if (statement.return_.value.kind == expression_invalid) return err;
    return statement;
}

Statement sem_statement_expression_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_expression, "should have found an expression");
    Statement err = {0};
    Statement statement = {0};
    statement.ast = ast;
    statement.type = statement_expression;
    statement.expression.value = sem_expression_parse(ast->expression.value, scope, templated_function);
    if (statement.expression.value.kind == expression_invalid) return err;
    return statement;
}

Statement sem_statement_variable_declaration_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_variable_declaration, "should have found a variable declaration");
    Statement err = {0};
    Statement statement = {0};
    statement.ast = ast;
    statement.type = statement_variable_declaration;

    Variable variable;
    variable.type = sem_type_parse(ast->variable_declaration.type, &templated_function->allocator_connection_map, scope, &templated_function->allocator_id_counter);
    if (variable.type.base->kind == type_invalid) return err;
    variable.name = ast->variable_declaration.name;
    variable.ast = ast;

    Variable* in_scope = sem_scope_add_variable(scope, &variable, templated_function);
    if (in_scope == NULL) return err;
    statement.variable_declaration.variable = in_scope;

    if (ast->variable_declaration.assignment != NULL) {
        Statement assignment = sem_statement_assignment_parse(ast->variable_declaration.assignment, scope, templated_function);
        if (assignment.type == statement_invalid) return err;
        statement.variable_declaration.assignment = alloc(sizeof(Statement));
        *statement.variable_declaration.assignment = assignment;
    } else {
        statement.variable_declaration.assignment = NULL;
    }

    return statement;
}

Expression sem_expression_value_access_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_value_access, "should have found a value access");
    char* access_name = ast->value_access.access_name;
    Expression expr = sem_expression_parse(ast->value_access.expr, scope, templated_function);
    if (expr.kind == expression_invalid) return expr;
    Type* expr_type = &expr.type;
    Type_Kind type_kind = sem_get_type_kind(expr_type);
    if (type_kind == type_type && strcmp(access_name, "size") == 0)
        return sem_expression_type_size_parse(ast, scope, templated_function, &expr);
    if (type_kind == type_type && strcmp(access_name, "align") == 0)
        return sem_expression_type_align_parse(ast, scope, templated_function, &expr);

    if (expr_type->base->kind == type_struct && expr_type->ptr_count <= 1)
        return sem_expression_struct_access_parse(ast, scope, templated_function, &expr);

    log_error_ast(ast, "could not find value access %s", access_name);
    return (Expression){0};
}

Expression sem_expression_struct_access_parse(Ast* ast, Scope* scope, Templated_Function* templated_function, Expression* struct_expr) {
    massert(ast->kind == ast_value_access, "should have found a value access");
    massert(struct_expr->type.base->kind == type_struct, "should have found a struct");
    massert(struct_expr->type.ptr_count <= 1, "should have found a struct");

    while (struct_expr->type.ptr_count != 0) {
        *struct_expr = sem_expression_dereference(struct_expr, templated_function);
    }

    char* access_name = ast->value_access.access_name;
    bool err = false;
    u64 field_index = get_string_uint(access_name, &err);
    if (field_index >= UINT32_MAX) {
        log_error_ast(ast, "field index to big");
        return (Expression){0};
    }
    if (err) {
        for (u32 i = 0; i < struct_expr->type.base->struct_.fields.count; i++) {
            Struct_Field* field = &struct_expr->type.base->struct_.fields.data[i];
            if (strcmp(field->name, access_name) == 0) {
                field_index = i;
                break;
            }
        }
    }
    Struct_Field* field = &struct_expr->type.base->struct_.fields.data[field_index];
    Type expr_type = sem_copy_type(&field->type);
    if (field->base_allocator_info.field == NULL) {
        expr_type.base_allocator_id = struct_expr->type.base_allocator_id;
    } else {
        massert(false, "not implemented");
    }
    for (u32 i = 0; i < struct_expr->type.ptr_count; i++) {
        if (field->ptr_allocator_infos.data[i].field == NULL) {
            expr_type.ptr_allocator_ids[i] = struct_expr->type.ptr_allocator_ids[i];
        } else {
            massert(false, "not implemented");
        }
    }
    if (struct_expr->type.is_ref) {
        expr_type.is_ref = true;
        expr_type.ref_allocator_id = struct_expr->type.ref_allocator_id;
    }

    Expression expression = {0};
    expression.ast = ast;
    expression.kind = expression_struct_access;
    expression.struct_access.expression = alloc(sizeof(Expression));
    *expression.struct_access.expression = *struct_expr;
    expression.struct_access.field_index = field_index;
    expression.type = expr_type;
    return expression;
}

Expression sem_expression_type_size_parse(Ast* ast, Scope* scope, Templated_Function* templated_function, Expression* type) {
    massert(ast->kind == ast_value_access, "should have found a value access");
    char* access_name = ast->value_access.access_name;
    massert(strcmp(access_name, "size") == 0, "should have found a size access");
    Expression expression = {0};
    expression.ast = ast;
    expression.kind = expression_type_size;
    if (type->type.base->kind != type_type) {
        log_error_ast(ast->value_access.expr, "expected type");
        return (Expression){0};
    }

    Type type_ = type->expression_type.type;
    expression.type_size.type = type_;
    expression.type = sem_type_get_const_int(&templated_function->allocator_id_counter);
    return expression;
}

Expression sem_expression_type_align_parse(Ast* ast, Scope* scope, Templated_Function* templated_function, Expression* type) {
    massert(ast->kind == ast_value_access, "should have found a value access");
    char* access_name = ast->value_access.access_name;
    massert(strcmp(access_name, "align") == 0, "should have found a align access");

    Expression expression = {0};
    expression.ast = ast;
    expression.kind = expression_type_align;
    if (type->type.base->kind != type_type) {
        log_error_ast(ast->value_access.expr, "expected type");
        return (Expression){0};
    }

    Type type_ = type->expression_type.type;
    expression.type_align.type = type_;
    expression.type = sem_type_get_const_int(&templated_function->allocator_id_counter);
    return expression;
}

Expression sem_expression_ptr_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_ptr, "should have found a value access");
    Expression expression = {0};
    expression.ast = ast;
    expression.kind = expression_ptr;
    expression.ptr.expression = alloc(sizeof(Expression));
    *expression.ptr.expression = sem_expression_parse(ast->ptr.expression, scope, templated_function);
    if (expression.ptr.expression->kind == expression_invalid) return expression;
    if (!expression.ptr.expression->type.is_ref) {
        log_error_ast(ast->ptr.expression, "cannot get pointer to a non reference value");
        return (Expression){0};
    }
    Type ptr_type = sem_ptr_of_ref(&expression.ptr.expression->type);
    expression.type = ptr_type;
    return expression;
}

Expression sem_expression_get_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_get, "should have found a value access");
    Expression expression = {0};
    expression.ast = ast;
    expression.kind = expression_get;
    expression.get.expression = alloc(sizeof(Expression));
    *expression.get.expression = sem_expression_parse(ast->get.expression, scope, templated_function);
    if (expression.get.expression->kind == expression_invalid) return expression;

    // Special case for type parsing
    if (expression.get.expression->kind == expression_type) {
        expression.get.expression->expression_type.type = sem_pointer_of_type(&expression.get.expression->expression_type.type, &templated_function->allocator_id_counter);
        expression.get.expression->ast->token_start[expression.get.expression->ast->num_tokens - 1].end_char++;
        return *expression.get.expression;
    }

    if (expression.get.expression->type.ptr_count == 0) {
        log_error_ast(ast->get.expression, "cannot get value of a non pointer value");
        return (Expression){0};
    }
    if (expression.get.expression->type.is_ref) {
        *expression.get.expression = sem_expression_dereference(expression.get.expression, templated_function);
        if (expression.get.expression->kind == expression_invalid) return *expression.get.expression;
    }
    Type deref_type = sem_dereference_type(&expression.get.expression->type);
    expression.type = deref_type;
    return expression;
}

Expression sem_expression_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    Ast* value_ast;
    if (ast->kind == ast_expression) {
        value_ast = ast->expression.value;
    } else {
        value_ast = ast;
    }
    switch (value_ast->kind) {
        case ast_int:
            return sem_expression_int_parse(value_ast, scope, templated_function);
        case ast_float:
            return sem_expression_float_parse(value_ast, scope, templated_function);
        case ast_variable:
            return sem_expression_variable_parse(value_ast, scope, templated_function);
        case ast_value_access:
            return sem_expression_value_access_parse(value_ast, scope, templated_function);
        case ast_alloc:
            return sem_expression_alloc_parse(value_ast, scope, templated_function);
        case ast_function_call:
            return sem_expression_function_call_parse(value_ast, scope, templated_function);
        case ast_type:
            return sem_expression_type_parse(value_ast, scope, templated_function);
        case ast_get:
            return sem_expression_get_parse(value_ast, scope, templated_function);
        case ast_ptr:
            return sem_expression_ptr_parse(value_ast, scope, templated_function);
        case ast_add:
            massert(false, "not implemented");
        case ast_sub:
            massert(false, "not implemented");
        case ast_mul:
            massert(false, "not implemented");
        case ast_div:
            massert(false, "not implemented");
        case ast_mod:
            massert(false, "not implemented");
        case ast_bit_and:
            massert(false, "not implemented");
        case ast_bit_or:
            massert(false, "not implemented");
        case ast_bit_xor:
            massert(false, "not implemented");
        case ast_bit_not:
            massert(false, "not implemented");
        case ast_bit_shl:
            massert(false, "not implemented");
        case ast_bit_shr:
            massert(false, "not implemented");
        case ast_and:
            massert(false, "not implemented");
        case ast_or:
            massert(false, "not implemented");
        case ast_equals_equals:
            massert(false, "not implemented");
        case ast_not_equals:
            massert(false, "not implemented");
        case ast_less_than:
            massert(false, "not implemented");
        case ast_greater_than:
            massert(false, "not implemented");
        case ast_less_than_equals:
            massert(false, "not implemented");
        case ast_greater_than_equals:
            massert(false, "not implemented");
        case ast_allocator:
        case ast_struct:
        case ast_struct_body:
        case ast_struct_field:
        case ast_function_call_parameter:
        case ast_variable_declaration:
        case ast_invalid:
        case ast_program:
        case ast_top_level:
        case ast_function_declaration:
        case ast_declaration_parameter_list:
        case ast_declaration_parameter:
        case ast_body:
        case ast_assignment:
        case ast_return:
        case ast_expression: {
            massert(false, "should never happen");
            Expression expression = {0};
            return expression;
        }
    }
}

Expression sem_expression_function_call_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_function_call, "should have found a function call");
    Expression_List parameters = {0};
    for (u32 i = 0; i < ast->function_call.parameters->function_call_parameter.parameters.count; i++) {
        Ast* parameter_ast = &ast->function_call.parameters->function_call_parameter.parameters.data[i];
        Expression parameter = sem_expression_parse(parameter_ast, scope, templated_function);
        if (parameter.kind == expression_invalid) return parameter;
        Expression_List_add(&parameters, &parameter);
    }
    char* name = ast->function_call.name;
    return sem_function_call(name, &parameters, ast, templated_function);
}

Expression sem_function_call(char* name, Expression_List* parameters, Ast* ast, Templated_Function* calling_templated_function) {
    Expression expression = {0};
    expression.ast = ast;
    expression.kind = expression_function_call;
    expression.function_call.parameters = *parameters;

    Function_Ptr_List function_to_test = {0};
    Function_Ptr_List passing_functions = {0};
    for (u32 i = 0; i < cap_context.functions.count; i++) {
        Function* function = cap_context.functions.data[i];
        if (strcmp(function->name, name) == 0) {
            if (function->parameters.count == parameters->count) {
                Function_Ptr_List_add(&function_to_test, &function);
            }
        }
    }

    for (u32 i = 0; i < function_to_test.count; i++) {
        Function* function = *Function_Ptr_List_get(&function_to_test, i);
        bool is_match = true;
        for (u32 j = 0; j < function->parameters.count; j++) {
            Function_Parameter* parameter = Function_Parameter_List_get(&function->parameters, j);
            Type* function_parameter_type = &parameter->type;
            Type* parameter_type = &parameters->data[j].type;
            if (sem_type_is_equal_without_allocator_id(function_parameter_type, parameter_type)) continue;
            is_match = false;
            break;
        }
        if (is_match) {
            Function_Ptr_List_add(&passing_functions, &function);
        }
    }

    if (passing_functions.count == 0) {
        for (u32 i = 0; i < function_to_test.count; i++) {
            Function* function = *Function_Ptr_List_get(&function_to_test, i);
            bool is_match = true;
            for (u32 j = 0; j < function->parameters.count; j++) {
                Function_Parameter* parameter = Function_Parameter_List_get(&function->parameters, j);
                Type* function_parameter_type = &parameter->type;
                Type* parameter_type = &parameters->data[j].type;
                if (sem_type_is_equal_without_allocator_id(function_parameter_type, parameter_type)) continue;
                if (sem_can_implicitly_cast(parameter_type, function_parameter_type)) continue;
                is_match = false;
                break;
            }
            if (is_match) {
                Function_Ptr_List_add(&passing_functions, &function);
            }
        }
    }

    if (passing_functions.count == 0) {
        log_error_ast(ast, "could not find function to call: %s", name);
        return (Expression){0};
    }
    if (passing_functions.count > 1) {
        log_error_ast(ast, "found multiple functions to call: %s", name);
        for (u32 i = 0; i < passing_functions.count; i++) {
            Function* function = *Function_Ptr_List_get(&passing_functions, i);
            log_info_ast(ast, "could have meant this function", function->name);
        }
    }

    Function* function = *Function_Ptr_List_get(&passing_functions, 0);
    Templated_Function* templated_function = function->templated_functions.data[0];

    for (u32 i = 0; i < parameters->count; i++) {
        Expression* parameter = Expression_List_get(parameters, i);
        Type* func_parameter_type = &templated_function->parameter_scope.variables.data[i]->type;
        Allocator* func_parameter_allocator = sem_allocator_get(&templated_function->allocator_connection_map, func_parameter_type->base_allocator_id);
        Type to_cast_to = sem_dereference_type(func_parameter_type);
        to_cast_to = sem_type_copy_allocator_id(&to_cast_to, &parameter->type);
        Expression cast_parameter = sem_expression_implicit_cast(parameter, &to_cast_to, calling_templated_function);
        if (cast_parameter.kind == expression_invalid) return cast_parameter;
        *parameter = cast_parameter;
    }

    expression.function_call.templated_function = templated_function;

    // get return type and setup allocator id's
    expression.type = sem_copy_type(&templated_function->return_type);
    expression.type.base_allocator_id = calling_templated_function->allocator_id_counter;
    calling_templated_function->allocator_id_counter += 1;
    if (expression.type.is_ref) {
        expression.type.ref_allocator_id = calling_templated_function->allocator_id_counter;
        calling_templated_function->allocator_id_counter += 1;
    }
    for (u32 i = 0; i < expression.type.ptr_count; i++) {
        expression.type.ptr_allocator_ids[i] = calling_templated_function->allocator_id_counter;
        calling_templated_function->allocator_id_counter += 1;
    }

    return expression;
}

Expression sem_expression_alloc_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    massert(ast->kind == ast_alloc, "should have found an alloc");
    Expression err = {0};
    Expression expression = {0};
    expression.ast = ast;
    expression.kind = expression_alloc;

    Ast* params = ast->alloc.parameters;
    if (params->function_call_parameter.parameters.count != 1 && params->function_call_parameter.parameters.count != 2) {
        log_error_ast(ast, "alloc can only be called with 1 or 2 parameters");
        return err;
    }

    Expression type_or_count = sem_expression_parse(&params->function_call_parameter.parameters.data[0], scope, templated_function);
    if (type_or_count.kind == expression_invalid) return type_or_count;
    Type* type_or_count_type = &type_or_count.type;
    Type_Kind type_or_count_kind = sem_get_type_kind(type_or_count_type);
    bool is_type = type_or_count_kind == type_type;
    if (!is_type) {
        Type u64_type = sem_get_uint_type(64, &templated_function->allocator_id_counter);
        type_or_count = sem_expression_implicit_cast(&type_or_count, &u64_type, templated_function);
        if (type_or_count.kind == expression_invalid) return type_or_count;
    }
    expression.alloc.type_or_count = alloc(sizeof(Expression));
    *expression.alloc.type_or_count = type_or_count;

    if (params->function_call_parameter.parameters.count == 2) {
        Expression count_or_allignment = sem_expression_parse(&params->function_call_parameter.parameters.data[1], scope, templated_function);
        if (count_or_allignment.kind == expression_invalid) return count_or_allignment;
        if (is_type) {
            Type u64_type = sem_get_uint_type(64, &templated_function->allocator_id_counter);
            count_or_allignment = sem_expression_implicit_cast(&count_or_allignment, &u64_type, templated_function);
            if (count_or_allignment.kind == expression_invalid) return count_or_allignment;
        } else {
            Type u32_type = sem_get_uint_type(32, &templated_function->allocator_id_counter);
            count_or_allignment = sem_expression_implicit_cast(&count_or_allignment, &u32_type, templated_function);
            if (count_or_allignment.kind == expression_invalid) return count_or_allignment;
        }
        expression.alloc.count_or_allignment = alloc(sizeof(Expression));
        *expression.alloc.count_or_allignment = count_or_allignment;
    } else {
        if (!is_type) {
            log_error_ast(ast, "if calling alloc with size must also pass an alignment");
            return err;
        }
        expression.alloc.count_or_allignment = NULL;
    }

    if (is_type) {
        expression.type = sem_pointer_of_type(&expression.alloc.type_or_count->expression_type.type, &templated_function->allocator_id_counter);
    } else {
        Type void_type = sem_type_get_void(&templated_function->allocator_id_counter);
        expression.type = sem_pointer_of_type(&void_type, &templated_function->allocator_id_counter);
    }

    // Mark the allocator as used for alloc
    Allocator allocator = *sem_allocator_get(&templated_function->allocator_connection_map, expression.type.ptr_allocator_ids[expression.type.ptr_count - 1]);
    allocator.used_for_alloc_or_free = true;
    sem_set_allocator(&templated_function->allocator_connection_map, expression.type.ptr_allocator_ids[expression.type.ptr_count - 1], &allocator);

    expression.alloc.is_type_alloc = is_type;
    return expression;
}

Expression sem_expression_type_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    Expression err = {0};
    Expression expression = {0};
    expression.ast = ast;
    expression.kind = expression_type;
    expression.expression_type.type = sem_type_parse(ast, &templated_function->allocator_connection_map, scope, &templated_function->allocator_id_counter);
    if (expression.expression_type.type.base->kind == type_invalid) return err;
    expression.type = sem_type_get_type(&templated_function->allocator_id_counter);
    return expression;
}

Expression sem_expression_variable_parse(Ast* ast, Scope* scope, Templated_Function* templated_function) {
    Variable* variable = sem_scope_get_variable(scope, ast->variable.name);
    if (variable == NULL) {
        // try as type
        Ast* type_ast = alloc(sizeof(Ast));
        type_ast->kind = ast_type;
        type_ast->type.name = ast->variable.name;
        type_ast->num_tokens = ast->num_tokens;
        type_ast->token_start = ast->token_start;
        type_ast->type.base_allocator = alloc(sizeof(Ast));
        *type_ast->type.base_allocator = (Ast){0};

        Expression expression = sem_expression_type_parse(type_ast, scope, templated_function);
        if (expression.kind != expression_invalid) return expression;
        log_error_ast(ast, "could not find variable %s", ast->variable.name);
        return expression;
    }
    Expression expression = {0};
    expression.ast = ast;
    expression.type = variable->type;
    massert(variable->type.is_ref, "should have found a ref");
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

bool sem_can_implicitly_cast(Type* from_type_, Type* to_type_) {
    Type from_type = *from_type_;
    Type to_type = *to_type_;
    if (from_type.is_ref && !to_type.is_ref) {
        from_type = sem_dereference_type(&from_type);
    }
    if (sem_type_is_equal_without_allocator_id(&from_type, &to_type)) {
        return true;
    }

    Type_Kind from_kind = sem_get_type_kind(&from_type);
    Type_Kind to_kind = sem_get_type_kind(&to_type);

    if ((from_kind == type_int && to_kind == type_int) ||
        (from_kind == type_uint && to_kind == type_uint)) {
        u64 from_bit_size = from_type.base->number_bit_size;
        u64 to_bit_size = to_type.base->number_bit_size;
        if (from_bit_size < to_bit_size) {
            return true;
        }
    }
    if (from_kind == type_float && to_kind == type_float) {
        return true;
    }

    if (from_kind == type_const_float && to_kind == type_float) {
        return true;
    }

    if (from_kind == type_const_int && to_kind == type_int) {
        return true;
    }
    if (from_kind == type_const_int && to_kind == type_uint) {
        return true;
    }
    if (from_kind == type_const_int && to_kind == type_float) {
        return true;
    }

    if (from_kind == type_ptr && to_kind == type_ptr) {
        Type underlying_from_type = sem_underlying_type(&from_type);
        Type_Kind underlying_from_kind = sem_get_type_kind(&underlying_from_type);
        if (underlying_from_kind == type_void) {
            return true;
        }
    }

    return false;
}

Expression sem_expression_implicit_cast(Expression* expression, Type* type, Templated_Function* templated_function) {
    Type from_type = expression->type;
    Type to_type = *type;
    Expression expression_to_cast = *expression;
    if (from_type.is_ref && !to_type.is_ref) {
        expression_to_cast = sem_expression_dereference(expression, templated_function);
        if (expression_to_cast.kind == expression_invalid) return expression_to_cast;
    }

    if (!sem_can_implicitly_cast(&from_type, &to_type)) {
        char* from_type_name = sem_type_name(&from_type);
        char* to_type_name = sem_type_name(&to_type);
        log_error_ast(expression->ast, "could not implicitly cast from %s to %s", from_type_name, to_type_name);
        return (Expression){0};
    }

    Expression expression_new = sem_expression_cast(expression, &to_type, templated_function);
    if (expression_new.kind == expression_invalid) return expression_new;
    return expression_new;
}

Expression sem_expression_dereference(Expression* expression, Templated_Function* templated_function) {
    Expression expression_new = {0};
    expression_new.ast = expression->ast;
    expression_new.kind = expression_cast;
    Type deref_type = sem_dereference_type(&expression->type);
    expression_new.type = deref_type;
    expression_new.cast.expression = alloc(sizeof(Expression));
    *expression_new.cast.expression = *expression;
    return expression_new;
}

Expression sem_expression_cast(Expression* expression, Type* type, Templated_Function* templated_function) {
    Expression err = {0};
    Type* old_type = &expression->type;

    bool res = true;

    Type* type1 = &expression->type;
    Type* type2 = type;

    if (sem_base_allocator_matters(type1) || sem_base_allocator_matters(type2)) {
        if (!sem_add_allocator_id_connection(&templated_function->allocator_connection_map, type1->base_allocator_id, type2->base_allocator_id, expression->ast)) return err;
    }

    if (type1->is_ref && type2->is_ref) {
        if (!sem_add_allocator_id_connection(&templated_function->allocator_connection_map, type1->ref_allocator_id, type2->ref_allocator_id, expression->ast)) return err;
    }

    u64 largest_size = max(type1->ptr_count, type2->ptr_count);
    u64 smallest_size = min(type1->ptr_count, type2->ptr_count);
    u64 start_i = largest_size - smallest_size;
    for (u32 i = start_i; i < largest_size; i++) {
        u32 allocator_id1;
        u32 allocator_id2;
        if (type1->ptr_count > type2->ptr_count) {
            allocator_id1 = type1->ptr_allocator_ids[i];
            allocator_id2 = type2->ptr_allocator_ids[i - start_i];
        } else {
            allocator_id1 = type1->ptr_allocator_ids[i - start_i];
            allocator_id2 = type2->ptr_allocator_ids[i];
        }
        if (!sem_add_allocator_id_connection(&templated_function->allocator_connection_map, allocator_id1, allocator_id2, expression->ast)) return err;
    }

    Expression expression_new = {0};
    expression_new.ast = expression->ast;
    expression_new.kind = expression_cast;
    expression_new.type = *type;
    expression_new.cast.expression = alloc(sizeof(Expression));
    *expression_new.cast.expression = *expression;
    return expression_new;
}

static void sem_handel_graph_additions_if_needed(Allocator_Connection_Map* map, u32 allocator_id) {
    u32 number_of_allocators = map->allocators.count;
    if (number_of_allocators <= allocator_id) {
        for (u32 i = number_of_allocators; i <= allocator_id; i++) {
            Allocator base_allocator = {0};
            Allocator_List_add(&map->allocators, &base_allocator);

            u32_List base_connection = {0};
            u32_List_List_add(&map->allocator_id_connections, &base_connection);
        }
    }
}

bool sem_allocator_are_the_same(Allocator* allocator1, Allocator* allocator2) {
    return allocator1->variable == allocator2->variable;
}

bool sem_allocator_are_exactly_the_same(Allocator* allocator1, Allocator* allocator2) {
    return allocator1->variable == allocator2->variable && allocator1->used_for_alloc_or_free == allocator2->used_for_alloc_or_free && allocator1->is_function_value == allocator2->is_function_value && allocator1->scope == allocator2->scope;
}

void sem_mark_type_as_function_value(Templated_Function* templated_function, Type* type, bool is_return_value) {
    if (sem_base_allocator_matters(type)) {
        u32 allocator_id = type->base_allocator_id;
        Allocator new_allocator = *sem_allocator_get(&templated_function->allocator_connection_map, allocator_id);
        new_allocator.is_function_value = true;
        sem_set_allocator(&templated_function->allocator_connection_map, allocator_id, &new_allocator);
    }
    u64 loop_size = type->ptr_count;
    if (!is_return_value && loop_size > 0) loop_size -= 1;
    for (u32 i = 0; i < loop_size; i++) {
        u32 allocator_id = type->ptr_allocator_ids[i];
        Allocator new_allocator = *sem_allocator_get(&templated_function->allocator_connection_map, allocator_id);
        new_allocator.is_function_value = true;
        sem_set_allocator(&templated_function->allocator_connection_map, allocator_id, &new_allocator);
    }
}

bool sem_scope_is_child_of(Scope* scope, Scope* parent) {
    while (scope != NULL) {
        if (scope == parent) return true;
        scope = scope->parent;
    }
    return false;
}

bool sem_add_allocator_id_connection(Allocator_Connection_Map* map, u32 allocator_id1, u32 allocator_id2, Ast* ast) {
    sem_handel_graph_additions_if_needed(map, allocator_id1);

    Allocator* allocator1 = sem_allocator_get(map, allocator_id1);
    Allocator* allocator2 = sem_allocator_get(map, allocator_id2);

    if (!sem_allocator_are_exactly_the_same(allocator1, allocator2)) {
        Allocator new_allocator = {0};
        new_allocator.used_for_alloc_or_free = allocator1->used_for_alloc_or_free || allocator2->used_for_alloc_or_free;
        new_allocator.is_function_value = allocator1->is_function_value || allocator2->is_function_value;
        if (sem_allocator_are_the_same(allocator1, allocator2)) {
            new_allocator.variable = allocator1->variable;
            massert(allocator1->scope == allocator2->scope, "should have found the same scope");
        } else if (sem_allocator_are_the_same(allocator1, &UNSPECIFIED_ALLOCATOR)) {
            new_allocator.variable = allocator2->variable;
            if (sem_scope_is_child_of(allocator1->scope, allocator2->scope) || allocator2->scope == NULL) {
                new_allocator.scope = allocator2->scope;
            } else {
                log_error_ast(ast, "cannot connect allocators becuase allocator would go out of scope");
                return false;
            }
        } else if (sem_allocator_are_the_same(allocator2, &UNSPECIFIED_ALLOCATOR)) {
            new_allocator.variable = allocator1->variable;
            if (sem_scope_is_child_of(allocator2->scope, allocator1->scope) || allocator1->scope == NULL) {
                new_allocator.scope = allocator1->scope;
            } else {
                log_error_ast(ast, "cannot connect allocators becuase allocator would go out of scope");
                return false;
            }
        } else {
            log_error_ast(ast, "cannot connect allocators becuase they are not the same or unspecified");
            return false;
        }

        if (new_allocator.is_function_value) {
            if (!sem_allocator_are_the_same(&new_allocator, &UNSPECIFIED_ALLOCATOR)) {
                log_error_ast(ast, "value's allocator must be unspecified(becuase it is returned up to the caller)");
                return false;
            }
        }

        sem_set_allocator(map, allocator_id1, &new_allocator);
        sem_set_allocator(map, allocator_id2, &new_allocator);
    }

    u32_List* connections1 = &map->allocator_id_connections.data[allocator_id1];
    u32_List_add(connections1, &allocator_id2);

    u32_List* connections2 = &map->allocator_id_connections.data[allocator_id2];
    u32_List_add(connections2, &allocator_id1);

    return true;
}

void sem_set_allocator(Allocator_Connection_Map* map, u32 allocator_id, Allocator* allocator) {
    sem_handel_graph_additions_if_needed(map, allocator_id);

    Allocator* existing_allocator = sem_allocator_get(map, allocator_id);
    if (sem_allocator_are_exactly_the_same(existing_allocator, allocator)) return;

    *Allocator_List_get(&map->allocators, allocator_id) = *allocator;
    u32_List connections = map->allocator_id_connections.data[allocator_id];
    for (u32 i = 0; i < connections.count; i++) {
        u32 connection = *u32_List_get(&connections, i);
        Allocator* connection_allocator = sem_allocator_get(map, connection);
        sem_set_allocator(map, connection, allocator);
    }
}

Allocator* sem_allocator_get(Allocator_Connection_Map* map, u32 allocator_id) {
    sem_handel_graph_additions_if_needed(map, allocator_id);
    return &map->allocators.data[allocator_id];
}

Type sem_type_copy_allocator_id(Type* to_type, Type* from_type) {
    Type new_type = sem_copy_type(to_type);
    new_type.base_allocator_id = from_type->base_allocator_id;
    new_type.ref_allocator_id = from_type->ref_allocator_id;
    massert(from_type->ptr_count == to_type->ptr_count, "should have found the same number of pointers");
    for (u32 i = 0; i < from_type->ptr_count; i++) {
        new_type.ptr_allocator_ids[i] = from_type->ptr_allocator_ids[i];
    }
    return new_type;
}

Type sem_copy_type(Type* type) {
    Type new_type = {0};
    new_type.base = type->base;
    new_type.base_allocator_id = type->base_allocator_id;
    new_type.ref_allocator_id = type->ref_allocator_id;
    new_type.is_ref = type->is_ref;
    new_type.ptr_count = type->ptr_count;
    new_type.ptr_allocator_ids = alloc(sizeof(u32) * type->ptr_count);
    memcpy(new_type.ptr_allocator_ids, type->ptr_allocator_ids, sizeof(u32) * type->ptr_count);
    return new_type;
}

Type sem_dereference_type(Type* type) {
    Type deref_type = *type;
    if (type->is_ref) {
        deref_type.is_ref = false;
        return deref_type;
    } else {
        massert(type->ptr_count > 0, "should have found a pointer");
        deref_type.is_ref = true;
        deref_type.ptr_count -= 1;
        deref_type.ref_allocator_id = type->ptr_allocator_ids[type->ptr_count - 1];
        return deref_type;
    }
}

Type sem_pointer_of_type(Type* type, u32* allocator_id_counter) {
    massert(!type->is_ref, "should not have found a ref");

    u32 allocator_id = *allocator_id_counter;
    allocator_id_counter += 1;

    Type pointer_of_type = *type;
    pointer_of_type.ptr_count += 1;
    u32* old_allocator_ids = pointer_of_type.ptr_allocator_ids;
    pointer_of_type.ptr_allocator_ids = alloc(sizeof(u32) * pointer_of_type.ptr_count);
    memcpy(pointer_of_type.ptr_allocator_ids, old_allocator_ids, sizeof(u32) * type->ptr_count);
    pointer_of_type.ptr_allocator_ids[pointer_of_type.ptr_count - 1] = allocator_id;
    return pointer_of_type;
}

Type sem_ptr_of_ref(Type* type) {
    massert(type->is_ref, "should have found a ref");
    Type ptr_of_type = *type;
    ptr_of_type.is_ref = false;
    ptr_of_type.ptr_count += 1;
    ptr_of_type.ptr_allocator_ids = alloc(sizeof(u32) * ptr_of_type.ptr_count);
    memcpy(ptr_of_type.ptr_allocator_ids, type->ptr_allocator_ids, sizeof(u32) * type->ptr_count);
    ptr_of_type.ptr_allocator_ids[ptr_of_type.ptr_count - 1] = type->ref_allocator_id;
    return ptr_of_type;
}

Type sem_underlying_type(Type* type) {
    massert(type->ptr_count > 0, "should have found a pointer");
    Type underlying_type = *type;
    underlying_type.ptr_count -= 1;
    underlying_type.is_ref = false;
    return underlying_type;
}

char* sem_type_name(Type* type) {
    char* base_name = type->base->name;
    u64 base_name_length = strlen(base_name);
    u64 added_symbols = type->ptr_count + type->is_ref;
    char* type_name = alloc(base_name_length + added_symbols + 1);
    memcpy(type_name, base_name, base_name_length);
    for (u32 i = 0; i < type->ptr_count; i++) {
        type_name[base_name_length] = '*';
        base_name_length += 1;
    }
    if (type->is_ref) {
        type_name[base_name_length] = '&';
        base_name_length += 1;
    }
    type_name[base_name_length] = '\0';
    return type_name;
}

bool sem_base_allocator_matters(Type* type) {
    return false;
}

bool sem_type_is_equal_without_allocator_id(Type* type1, Type* type2) {
    bool base_is_equal = type1->base == type2->base;
    bool ptr_count_is_equal = type1->ptr_count == type2->ptr_count;
    bool is_ref_is_equal = type1->is_ref == type2->is_ref;
    return base_is_equal && ptr_count_is_equal && is_ref_is_equal;
}

Type_Kind sem_get_type_kind(Type* type) {
    if (type->is_ref) return type_ref;
    if (type->ptr_count > 0) return type_ptr;
    return type->base->kind;
}

Allocator_Connection_Map sem_copy_allocator_connection_map(Allocator_Connection_Map* map) {
    Allocator_Connection_Map new_map = {0};
    new_map.allocators = map->allocators;
    new_map.allocators.capacity = new_map.allocators.count;
    new_map.allocators.data = alloc(sizeof(Allocator) * new_map.allocators.capacity);
    memcpy(new_map.allocators.data, map->allocators.data, sizeof(Allocator) * new_map.allocators.capacity);

    new_map.allocator_id_connections = map->allocator_id_connections;
    new_map.allocator_id_connections.capacity = new_map.allocator_id_connections.count;
    new_map.allocator_id_connections.data = alloc(sizeof(u32_List) * new_map.allocator_id_connections.capacity);
    memcpy(new_map.allocator_id_connections.data, map->allocator_id_connections.data, sizeof(u32_List) * new_map.allocator_id_connections.capacity);

    for (u32 i = 0; i < new_map.allocators.capacity; i++) {
        new_map.allocator_id_connections.data[i].capacity = new_map.allocator_id_connections.data[i].capacity;
        new_map.allocator_id_connections.data[i].data = alloc(sizeof(u32) * new_map.allocator_id_connections.data[i].capacity);
        memcpy(new_map.allocator_id_connections.data[i].data, map->allocator_id_connections.data[i].data, sizeof(u32) * new_map.allocator_id_connections.data[i].capacity);
    }

    return new_map;
}

bool sem_allocator_expression_is_valid(Expression* expression) {
    switch (expression->kind) {
        case expression_cast:
            return sem_allocator_expression_is_valid(expression->cast.expression);
        case expression_ptr:
            return sem_allocator_expression_is_valid(expression->ptr.expression);
        case expression_struct_access:
            return sem_allocator_expression_is_valid(expression->struct_access.expression);
        case expression_get:
            return sem_allocator_expression_is_valid(expression->get.expression);
        case expression_variable:
            return true;
        case expression_type_size:
        case expression_type_align:
        case expression_type:
        case expression_function_call:
        case expression_invalid:
        case expression_alloc:
        case expression_int:
        case expression_float:
            return false;
    }
}

bool sem_allocator_expression_is_equal(Expression* expression1, Expression* expression2) {
    if (expression1->kind != expression2->kind) return false;
    if (!sem_type_is_equal_without_allocator_id(&expression1->type, &expression2->type)) return false;
    switch (expression1->kind) {
        case expression_cast: {
            return sem_allocator_expression_is_equal(expression1->cast.expression, expression2->cast.expression);
        }
        case expression_ptr: {
            return sem_allocator_expression_is_equal(expression1->ptr.expression, expression2->ptr.expression);
        }
        case expression_struct_access: {
            u32 field_index1 = expression1->struct_access.field_index;
            u32 field_index2 = expression2->struct_access.field_index;
            if (field_index1 != field_index2) return false;
            return sem_allocator_expression_is_equal(expression1->struct_access.expression, expression2->struct_access.expression);
        }
        case expression_get: {
            return sem_allocator_expression_is_equal(expression1->get.expression, expression2->get.expression);
        }
        case expression_variable: {
            Variable* var1 = expression1->variable.variable;
            Variable* var2 = expression2->variable.variable;
            return var1 == var2;
        }
        case expression_type_size:
        case expression_type_align:
        case expression_type:
        case expression_function_call:
        case expression_invalid:
        case expression_alloc:
        case expression_int:
        case expression_float:
            massert(false, "these expressions should never be allowed for allocators");
    }
}
