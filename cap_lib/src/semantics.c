#include "cap.h"
#include "cap/ast.h"

String sem_type_to_string(Type* type) {
    switch (type->kind) {
        case type_function: {
            Function* function = type->function;
        }
        case type_type: {
            return str("type");
        }
        case type_invalid:
            return str("invalid");
        case type_pointer: {
            String underlying_type_str = sem_type_to_string(type->pointer.underlying_type);
            String str = {0};
            char buffer[2048];
            snprintf(buffer, 2048, "*%.*s", str_info(underlying_type_str));
            u64 length = strlen(buffer);
            char* ptr = cap_alloc(length + 1);
            memcpy(ptr, buffer, length);
            return string_create(ptr, length);
        }
        case type_reference: {
            String underlying_type_str = sem_type_to_string(type->reference.underlying_type);
            String str = {0};
            char buffer[2048];
            snprintf(buffer, 2048, "&%.*s", str_info(underlying_type_str));
            u64 length = strlen(buffer);
            char* ptr = cap_alloc(length + 1);
            memcpy(ptr, buffer, length);
            return string_create(ptr, length);
        }
        case type_int: {
            i64 bits = type->int_.bits;
            String number_str = string_int(bits);
            String str = str("i");
            str = string_append(str, number_str);
            return str;
        }
        case type_float: {
            i64 bits = type->float_.bits;
            String number_str = string_float(bits);
            String str = str("f");
            str = string_append(str, number_str);
            return str;
        }
        case type_bool: {
            i64 bits = type->bool_.bits;
            String number_str = string_int(bits);
            String str = str("b");
            str = string_append(str, number_str);
            return str;
        }
        case type_uint: {
            i64 bits = type->uint.bits;
            String number_str = string_int(bits);
            String str = str("u");
            str = string_append(str, number_str);
            return str;
        }
        case type_void: {
            return str("void");
        }
    }
}

Type sem_type_type() {
    Type type = {0};
    type.kind = type_type;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_void_type() {
    Type type = {0};
    type.kind = type_void;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_int_type(i64 bits) {
    Type type = {0};
    type.kind = type_int;
    type.int_.bits = bits;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_uint_type(i64 bits) {
    Type type = {0};
    type.kind = type_uint;
    type.uint.bits = bits;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_bool_type(i64 bits) {
    Type type = {0};
    type.kind = type_bool;
    type.bool_.bits = bits;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_float_type(i64 bits) {
    Type type = {0};
    type.kind = type_float;
    type.float_.bits = bits;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_type_parse(Ast* ast, Scope* scope) {
    switch (ast->kind) {
        case ast_dereference: {
            Type underlying_type = sem_type_parse(ast->dereference.value, scope);
            if (underlying_type.kind == type_invalid) return (Type){0};
            if (underlying_type.kind == type_reference) {
                log_error_ast(ast, "cannot have a pointer to a reference");
                return (Type){0};
            }
            Type type = {0};
            type.kind = type_pointer;
            type.allocator_id = sem_get_new_allocator_id();
            type.pointer.underlying_type = cap_alloc(sizeof(Type));
            *type.pointer.underlying_type = underlying_type;
            return type;
        }
        case ast_reference: {
            Type underlying_type = sem_type_parse(ast->reference.value, scope);
            if (underlying_type.kind == type_invalid) return (Type){0};
            if (underlying_type.kind == type_reference) {
                log_error_ast(ast, "cannot have a reference to a reference");
                return (Type){0};
            }
            Type type = {0};
            type.kind = type_reference;
            type.allocator_id = sem_get_new_allocator_id();
            type.reference.underlying_type = cap_alloc(sizeof(Type));
            *type.reference.underlying_type = underlying_type;
            return type;
        }
        case ast_variable: {
            String var_name = ast->variable.name;
            Variable* var = sem_find_variable(var_name, scope);
            if (var != NULL) {
                Type* underlying_type = var->underlying_type;
                massert(underlying_type->kind != type_type, str("underlying type should never be type_type"));
                return *underlying_type;
            }
            if (string_equal(var_name, str("void"))) return sem_void_type();
            if (string_equal(var_name, str("type"))) return sem_type_type();
            char buffer[2048];
            memcpy(buffer, var_name.data, var_name.length);
            buffer[var_name.length] = 0;
            i64 bits = 0;
            if (sscanf(buffer, "i%lld", &bits) == 1) return sem_int_type(bits);
            if (sscanf(buffer, "f%lld", &bits) == 1) return sem_float_type(bits);
            if (sscanf(buffer, "b%lld", &bits) == 1) return sem_bool_type(bits);
            if (sscanf(buffer, "u%lld", &bits) == 1) return sem_uint_type(bits);
            log_error_ast(ast, "could not find type called %.*s", str_info(var_name));
            return (Type){0};
        }
        default: {
            log_error_ast(ast, "unexpected ast kind in type parse");
            return (Type){0};
        }
    }
}

Program sem_program_parse(Ast* ast) {
    Program program = {0};
    program.name = ast->program.name;
    Type return_type = sem_int_type(32);
    String function_name = str("main");
    Function function = sem_create_function(function_name, &return_type, 1, NULL, 0, ast);
    program.function = function;
    return program;
}

Function sem_function_parse(Ast* ast) {
    massert(ast->kind == ast_function_declaration, str("expected function declaration"));
    Ast_Function_Declaration* function_declaration = &ast->function_declaration;

    String name = function_declaration->name;

    u64 return_types_capacity = 8;
    Type* return_types = cap_alloc(return_types_capacity * sizeof(Type));
    u64 return_types_count = 0;

    for (u64 i = 0; i < function_declaration->return_types_count; i++) {
        Ast* return_type = &function_declaration->return_types[i];
        Type type = sem_type_parse(return_type, &cap_context.global_scope);
        ptr_append(return_types, return_types_count, return_types_capacity, type);
    }

    u64 parameters_capacity = 8;
    Function_Parameter* parameters = cap_alloc(parameters_capacity * sizeof(Function_Parameter));
    u64 parameters_count = 0;

    for (u64 i = 0; i < function_declaration->parameters_count; i++) {
        Ast* parameter = &function_declaration->parameters[i];
        massert(parameter->kind == ast_function_declaration_parameter, str("expected function declaration parameter"));
        String name = parameter->function_declaration_parameters.name;
        Ast* type_ast = parameter->function_declaration_parameters.type;
        Type type = sem_type_parse(type_ast, &cap_context.global_scope);
        Function_Parameter function_parameter = {0};
        function_parameter.name = name;
        function_parameter.type = type;
        ptr_append(parameters, parameters_count, parameters_capacity, function_parameter);
    }
    return sem_create_function(name, return_types, return_types_count, parameters, parameters_count, ast);
}

Function sem_create_function(String name, Type* return_types, u64 return_types_count, Function_Parameter* parameters, u64 parameters_count, Ast* function_ast) {
    Function function = {0};
    function.name = name;
    function.return_types = return_types;
    function.return_types_count = return_types_count;
    function.parameters = parameters;
    function.parameters_count = parameters_count;
    function.implementations_capacity = 1;
    function.implementations_count = 0;
    function.implementations = cap_alloc(sizeof(Function_Implementation));
    function.ast = function_ast;
    return function;
}

Allocator_Id sem_get_new_allocator_id() {
    Allocator_Id id = cap_context.allocator_map.id_counter;
    cap_context.allocator_map.id_counter++;

    Allocator_Id_Data info = {0};
    info.allocator_index = 0;
    info.connected_ids_capacity = 8;
    info.connected_ids = cap_alloc(info.connected_ids_capacity * sizeof(Allocator_Id));
    info.connected_ids_count = 0;

    ptr_append(cap_context.allocator_map.data, cap_context.allocator_map.data_count, cap_context.allocator_map.data_capacity, info);

    return id;
}

Variable* sem_find_variable(String name, Scope* scope) {
    for (u64 i = 0; i < scope->variables_count; i++) {
        Variable** variable = &scope->variables[i];
        if (string_equal((*variable)->name, name)) return *variable;
    }
    if (scope->parent != NULL) {
        return sem_find_variable(name, scope->parent);
    }
    return NULL;
}

Variable* sem_add_variable_to_scope(String name, Type* type, Scope* scope) {
    Variable* variable = cap_alloc(sizeof(Variable));
    variable->name = name;
    variable->type = *type;
    ptr_append(scope->variables, scope->variables_count, scope->variables_capacity, variable);
    return variable;
}
