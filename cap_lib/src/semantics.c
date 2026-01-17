#include "cap/semantics.h"

#include "cap.h"
#include "cap/ast.h"

String sem_type_to_string(Type* type) {
    switch (type->kind) {
        case type_int_literal: {
            return str("int_literal");
        }
        case type_float_literal: {
            return str("float_literal");
        }
        case type_function: {
            Type_Function* function = &type->function;
            String str = {0};
            for (u64 i = 0; i < function->return_types_count; i++) {
                Type* return_type = &function->return_types[i];
                String return_type_str = sem_type_to_string(return_type);
                str = string_append(str, return_type_str);
                if (i != function->return_types_count - 1) str = string_append(str, str(", "));
            }
            str = string_append(str, str("func("));
            for (u64 i = 0; i < function->parameter_types_count; i++) {
                Type* parameter_type = &function->parameter_types[i];
                String parameter_type_str = sem_type_to_string(parameter_type);
                str = string_append(str, parameter_type_str);
                if (i != function->parameter_types_count - 1) str = string_append(str, str(", "));
            }
            str = string_append(str, str(")"));
            return str;
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
            String number_str = string_int(bits);
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
    type.is_complete = true;
    type.kind = type_type;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_void_type() {
    Type type = {0};
    type.is_complete = true;
    type.kind = type_void;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_int_type(i64 bits) {
    Type type = {0};
    type.is_complete = true;
    type.kind = type_int;
    type.int_.bits = bits;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_uint_type(i64 bits) {
    Type type = {0};
    type.is_complete = true;
    type.kind = type_uint;
    type.uint.bits = bits;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_bool_type(i64 bits) {
    Type type = {0};
    type.is_complete = true;
    type.kind = type_bool;
    type.bool_.bits = bits;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_float_type(i64 bits) {
    Type type = {0};
    type.is_complete = true;
    type.kind = type_float;
    type.float_.bits = bits;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_type_reference(Type* underlying_type, Ast* ast_for_error) {
    if (underlying_type->kind == type_type) {
        log_error_ast(ast_for_error, "can't have a reference to a type");
        return (Type){0};
    }
    if (underlying_type->kind == type_function) {
        log_error_ast(ast_for_error, "can't have a reference to a function");
        return (Type){0};
    }
    Type type = {0};
    type.kind = type_reference;
    type.is_complete = true;
    type.reference.underlying_type = underlying_type;
    type.allocator_id = sem_get_new_allocator_id();
    return type;
}

Type sem_type_pointer(Type* underlying_type, Ast* ast_for_error) {
    if (underlying_type->kind == type_type) {
        log_error_ast(ast_for_error, "can't have a pointer to a type");
        return (Type){0};
    }
    if (underlying_type->kind == type_function) {
        log_error_ast(ast_for_error, "can't have a pointer to a function");
        return (Type){0};
    }
    Type type = {0};
    type.kind = type_pointer;
    type.is_complete = true;
    type.pointer.underlying_type = underlying_type;
    type.allocator_id = sem_get_new_allocator_id();
    return type;
}

Type sem_type_int_literal() {
    Type type = {0};
    type.kind = type_int_literal;
    type.is_complete = true;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_type_float_literal() {
    Type type = {0};
    type.kind = type_float_literal;
    type.is_complete = true;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_type_invalid() {
    Type type = {0};
    type.kind = type_invalid;
    type.is_complete = true;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_type_dereference(Type* type) {
    if (type->kind == type_type) return *type;
    massert(type->kind == type_pointer || type->kind == type_reference, str("expected pointer or reference"));
    if (type->kind == type_pointer) {
        Type new_type = *type;
        new_type.kind = type_reference;
        new_type.reference.underlying_type = type->pointer.underlying_type;
        return new_type;
    } else if (type->kind == type_reference) {
        return *type->reference.underlying_type;
    } else {
        mabort(str("expected pointer or reference"));
    }
}

Type sem_type_underlying_type(Type* type) {
    massert(type->kind == type_pointer || type->kind == type_reference, str("expected pointer or reference"));
    if (type->kind == type_pointer) {
        return *type->pointer.underlying_type;
    } else if (type->kind == type_reference) {
        return *type->reference.underlying_type;
    } else {
        mabort(str("expected pointer or reference"));
    }
}

Type sem_type_new_allocator_ids(Type* type) {
    switch (type->kind) {
        case type_function: {
            Type_Function* function = &type->function;
            Type_Function new_function = *function;
            new_function.return_types = cap_alloc(function->return_types_count * sizeof(Type));
            new_function.return_types_count = function->return_types_count;
            for (u64 i = 0; i < function->return_types_count; i++) {
                Type* return_type = &function->return_types[i];
                Type new_return_type = sem_type_new_allocator_ids(return_type);
                new_function.return_types[i] = new_return_type;
            }
            new_function.parameter_types = cap_alloc(function->parameter_types_count * sizeof(Type));
            new_function.parameter_types_count = function->parameter_types_count;
            for (u64 i = 0; i < function->parameter_types_count; i++) {
                Type* parameter_type = &function->parameter_types[i];
                Type new_parameter_type = sem_type_new_allocator_ids(parameter_type);
                new_function.parameter_types[i] = new_parameter_type;
            }
            Type new_type = *type;
            new_type.kind = type_function;
            new_type.function = new_function;
            new_type.allocator_id = sem_get_new_allocator_id();
            return new_type;
        }
        case type_pointer: {
            Type* underlying_type = type->pointer.underlying_type;
            Type new_type_underlying_type = sem_type_new_allocator_ids(underlying_type);
            Type new_type = *type;
            new_type.allocator_id = sem_get_new_allocator_id();
            new_type.pointer.underlying_type = cap_alloc(sizeof(Type));
            *new_type.pointer.underlying_type = new_type_underlying_type;
            return new_type;
        }
        case type_reference: {
            Type* underlying_type = type->reference.underlying_type;
            Type new_type_underlying_type = sem_type_new_allocator_ids(underlying_type);
            Type new_type = *type;
            new_type.allocator_id = sem_get_new_allocator_id();
            new_type.reference.underlying_type = cap_alloc(sizeof(Type));
            *new_type.reference.underlying_type = new_type_underlying_type;
            return new_type;
        }
        case type_int_literal:
        case type_float_literal:
        case type_type:
        case type_void:
        case type_bool:
        case type_float:
        case type_uint:
        case type_int:
        case type_invalid: {
            return *type;
        }
    }
}

bool sem_type_is_reference_of(Type* type, Type* underlying_type) {
    massert(type->kind == type_pointer, str("expected pointer"));
    Type ptr_underlying_type = sem_type_underlying_type(type);
    return sem_type_equal_without_allocator(&ptr_underlying_type, underlying_type);
}

bool sem_type_is_ptr_to(Type* type, Type* underlying_type) {
    massert(type->kind == type_reference, str("expected reference"));
    Type ref_underlying_type = sem_type_underlying_type(type);
    return sem_type_equal_without_allocator(&ref_underlying_type, underlying_type);
}

Type sem_function_type(Type* return_types, u64 return_types_count, Type* parameter_types, u64 parameter_types_count, Allocator_Id allocator_id) {
    Type type = {0};
    type.kind = type_function;
    type.is_complete = true;
    type.function.return_types = return_types;
    type.function.return_types_count = return_types_count;
    type.function.parameter_types = parameter_types;
    type.function.parameter_types_count = parameter_types_count;
    type.allocator_id = allocator_id;
    return type;
}

bool sem_type_allocator_equal(Type* type_a, Type* type_b) {
    if (type_a->kind != type_b->kind) return false;
    switch (type_a->kind) {
        case type_function: {
            // TODO: maybe do somthing here
            return false;
        }
        case type_pointer: {
            u64 allocator_id_a = type_a->allocator_id;
            u64 allocator_id_b = type_b->allocator_id;
            if (allocator_id_a != allocator_id_b) return false;
            Type* underlying_type_a = type_a->pointer.underlying_type;
            Type* underlying_type_b = type_b->pointer.underlying_type;
            return sem_type_allocator_equal(underlying_type_a, underlying_type_b);
        }
        case type_reference: {
            u64 allocator_id_a = type_a->allocator_id;
            u64 allocator_id_b = type_b->allocator_id;
            if (allocator_id_a != allocator_id_b) return false;
            Type* underlying_type_a = type_a->reference.underlying_type;
            Type* underlying_type_b = type_b->reference.underlying_type;
            return sem_type_allocator_equal(underlying_type_a, underlying_type_b);
        }
        case type_invalid:
        case type_int_literal:
        case type_float_literal:
        case type_int:
        case type_uint:
        case type_bool:
        case type_void:
        case type_type:
        case type_float: {
            return true;
        }
    }
}

bool sem_type_equal(Type* type_a, Type* type_b) {
    if (!sem_type_equal_without_allocator(type_a, type_b)) return false;
    return sem_type_allocator_equal(type_a, type_b);
}

bool sem_type_equal_without_allocator(Type* type_a, Type* type_b) {
    if (type_a->kind != type_b->kind) return false;
    switch (type_a->kind) {
        case type_function: {
            Type_Function* function_a = &type_a->function;
            Type_Function* function_b = &type_b->function;
            if (function_a->return_types_count != function_b->return_types_count) return false;
            if (function_a->parameter_types_count != function_b->parameter_types_count) return false;
            for (u64 i = 0; i < function_a->return_types_count; i++) {
                Type* return_type_a = &function_a->return_types[i];
                Type* return_type_b = &function_b->return_types[i];
                if (!sem_type_equal_without_allocator(return_type_a, return_type_b)) return false;
            }
            for (u64 i = 0; i < function_a->parameter_types_count; i++) {
                Type* parameter_type_a = &function_a->parameter_types[i];
                Type* parameter_type_b = &function_b->parameter_types[i];
                if (!sem_type_equal_without_allocator(parameter_type_a, parameter_type_b)) return false;
            }
            return true;
        }
        case type_float: {
            if (type_a->float_.bits != type_b->float_.bits) return false;
            return true;
        }
        case type_uint: {
            if (type_a->uint.bits != type_b->uint.bits) return false;
            return true;
        }
        case type_int: {
            if (type_a->int_.bits != type_b->int_.bits) return false;
            return true;
        }
        case type_bool: {
            if (type_a->bool_.bits != type_b->bool_.bits) return false;
            return true;
        }
        case type_reference: {
            Type* underlying_type_a = type_a->reference.underlying_type;
            Type* underlying_type_b = type_b->reference.underlying_type;
            return sem_type_equal_without_allocator(underlying_type_a, underlying_type_b);
        }
        case type_pointer: {
            Type* underlying_type_a = type_a->pointer.underlying_type;
            Type* underlying_type_b = type_b->pointer.underlying_type;
            return sem_type_equal_without_allocator(underlying_type_a, underlying_type_b);
        }
        case type_int_literal:
        case type_float_literal:
        case type_type:
        case type_void:
        case type_invalid:
            return true;
    }
}

Type sem_type_parse(Ast* ast) {
    Expression expr = sem_expression_parse(ast);
    if (expr.kind == expression_invalid) return (Type){0};
    if (expr.type.kind != type_type) {
        log_error_ast(ast, "expected type");
        return (Type){0};
    }
    return *expr.compile_time_value.underlying_type;
}

Function sem_function_parse(Ast* ast) {
    massert(ast->kind == ast_function_declaration, str("expected function declaration"));
    Ast_Function_Declaration* function_declaration = &ast->function_declaration;

    u64 parameters_capacity = 8;
    String* parameter_names = cap_alloc(parameters_capacity * sizeof(String));
    u64 parameters_count = 0;

    for (u64 i = 0; i < function_declaration->parameters_count; i++) {
        Ast* parameter = &function_declaration->parameters[i];
        massert(parameter->kind == ast_function_declaration_parameter, str("expected function declaration parameter"));
        String name = parameter->function_declaration_parameters.name;
        ptr_append(parameter_names, parameters_count, parameters_capacity, name);
    }

    Type function_type = sem_function_type(NULL, 0, NULL, 0, NO_ALLOCATOR_ID);
    function_type.is_complete = false;
    return sem_create_function(function_type, parameter_names, ast);
}

void sem_function_add_types(Function* function) {
    Ast* ast = function->ast;
    massert(ast->kind == ast_function_declaration, str("expected function declaration"));
}

Function sem_create_function(Type function_type, String* parameter_names, Ast* ast) {
    Function function = {0};
    function.function_type = function_type;
    function.parameter_names = parameter_names;
    function.scope_created_in = cap_context.scope;

    function.implementations_capacity = 1;
    function.implementations_count = 0;
    function.implementations = cap_alloc(sizeof(Function_Implementation));
    function.ast = ast;
    function.namespace_id = cap_context.namespace_we_are_in;
    return function;
}

Allocator_Id sem_get_new_allocator_id() {
    Allocator_Id id = cap_context.allocator_map.allocator_count;
    Allocator* new_allocator = cap_alloc(sizeof(Allocator));
    ptr_append(cap_context.allocator_map.allocator, cap_context.allocator_map.allocator_count, cap_context.allocator_map.allocator_capacity, new_allocator);
    return id;
}

void sem_connect_allocator_ids(Allocator_Id id1, Allocator_Id id2) {
    if (id1 == NO_ALLOCATOR_ID || id2 == NO_ALLOCATOR_ID) return;
    Allocator* data1 = cap_context.allocator_map.allocator[id1];
    Allocator* data2 = cap_context.allocator_map.allocator[id2];
    for (u64 i = 0; i < cap_context.allocator_map.allocator_count; i++) {
        Allocator** data = &cap_context.allocator_map.allocator[i];
        if (*data == data2) *data = data1;
    }
}

void sem_set_id_allocator(Allocator_Id id, Allocator* allocator) {
    *(cap_context.allocator_map.allocator[id]) = *allocator;
}

bool _sem_variable_fits_namespace(Variable* variable, String* namespaces, u64 namespaces_count, u64 namespace_we_are_in) {
    Cap_Folder* folder = cap_context.folders[namespace_we_are_in];
    if (variable->namespace == namespace_we_are_in && namespaces_count == 0) return true;
    for (u64 j = 0; j < folder->folders_count; j++) {
        Cap_Folder* child_folder = folder->folders[j];
        String alias = folder->folder_namespace_aliases[j];
        if (string_equal(alias, str(""))) {
            if (_sem_variable_fits_namespace(variable, namespaces, namespaces_count, child_folder->namespace_id)) return true;
        }
    }
    if (namespaces_count == 0) return false;
    String namespace_alias = namespaces[0];
    for (u64 j = 0; j < folder->folders_count; j++) {
        Cap_Folder* child_folder = folder->folders[j];
        String alias = folder->folder_namespace_aliases[j];
        if (string_equal(alias, namespace_alias)) {
            if (_sem_variable_fits_namespace(variable, namespaces + 1, namespaces_count - 1, child_folder->namespace_id)) return true;
        }
    }
    return false;
}

bool sem_variable_fits_namespace(Variable* variable, String* namespaces, u64 namespaces_count) {
    u64 namespace_we_are_in = cap_context.namespace_we_are_in;
    return _sem_variable_fits_namespace(variable, namespaces, namespaces_count, namespace_we_are_in);
}

Variable* __sem_find_variable(String name, String* namespaces, u64 namespaces_count, Ast* ast_for_error, Scope* scope) {
    u64 variables_count = 0;
    u64 variables_capacity = 8;
    Variable** variables = cap_alloc(variables_capacity * sizeof(Variable));
    for (u64 i = 0; i < scope->variables_count; i++) {
        Variable** variable = &scope->variables[i];
        if (string_equal((*variable)->name, name) && sem_variable_fits_namespace(*variable, namespaces, namespaces_count)) {
            u64 variable_namespace = (*variable)->namespace;
            ptr_append(variables, variables_count, variables_capacity, *variable);
        };
    }
    if (variables_count == 1) {
        return variables[0];
    } else if (variables_count > 1) {
        if (ast_for_error == NULL) return NULL;
        log_error_ast(ast_for_error, "variable is ambiguous");
        for (u64 i = 0; i < variables_count; i++) {
            Variable* variable = variables[i];
            log_info_ast(variable->ast, "could of meant");
        }
        return NULL;
    }
    if (scope->parent != NULL) {
        return __sem_find_variable(name, namespaces, namespaces_count, ast_for_error, scope->parent);
    }

    if (ast_for_error) log_error_ast(ast_for_error, "variable %.*s not found", str_info(name));
    return NULL;
}

Variable* sem_find_variable(String name, String* namespaces, u64 namespaces_count, Ast* ast_for_error) {
    return __sem_find_variable(name, namespaces, namespaces_count, ast_for_error, cap_context.scope);
}

Variable* sem_add_variable(String name, Type type, Ast* ast, Compile_Time_Value compile_time_value) {
    Variable variable = {0};
    variable.namespace = cap_context.namespace_we_are_in;
    variable.name = name;
    variable.type = type;
    variable.ast = ast;
    variable.compile_time_value = compile_time_value;

    Scope* scope = cap_context.scope;
    if (variable.type.kind == type_function) {
        // function operat on a different level with overrideing becuase of function overloading
        // so we don't just check if the function is already declared
    } else {
        Variable* existing = NULL;
        for (u64 i = 0; i < scope->variables_count; i++) {
            Variable* v = scope->variables[i];
            if (string_equal(v->name, variable.name) && sem_variable_fits_namespace(v, NULL, 0)) {
                existing = v;
                break;
            }
        }

        if (existing != NULL) {
            if (variable.type.kind != type_function || existing->type.kind != type_function) {
                log_error_ast(variable.ast, "variable %.*s already declared", str_info(variable.name));
                return NULL;
            }
        }
    }
    Variable* variable_ptr = cap_alloc(sizeof(Variable));
    *variable_ptr = variable;
    ptr_append(scope->variables, scope->variables_count, scope->variables_capacity, variable_ptr);
    return variable_ptr;
}

Function* sem_find_function(String name, String* namespaces, u64 namespaces_count, Type* parameters, u64 parameter_count, Ast* ast) {
    u64 functions_count = 0;
    Variable** functions = sem_find_functions_with_name_and_namespace(name, namespaces, namespaces_count, &functions_count);
    if (functions_count == 0) {
        log_error_ast(ast, "could not find any function %.*s", str_info(name));
        return NULL;
    }
    u64 function_matches_count = 0;
    Variable** function_matches = cap_alloc(functions_count * sizeof(Variable*));
    for (u64 i = 0; i < functions_count; i++) {
        Variable* function = functions[i];
        Type* function_type = &function->type;

        // TODO: maybe handel this better later
        cap_context.log = false;
        sem_complete_variable_type(function);
        if (!function->type.is_complete) {
            cap_context.log = true;
            continue;
        }
        cap_context.log = true;

        bool is_match = true;
        if (function_type->function.parameter_types_count != parameter_count) continue;
        for (u64 j = 0; j < parameter_count; j++) {
            Type* function_parameter_type = &function_type->function.parameter_types[j];
            Type* parameter_type = &parameters[j];
            if (!sem_type_equal_without_allocator(function_parameter_type, parameter_type)) {
                is_match = false;
                break;
            }
        }
        if (!is_match) continue;
        function_matches[function_matches_count] = function;
        function_matches_count += 1;
    }
    if (function_matches_count == 0) {
        log_error_ast(ast, "could not find any function with the given parameter types");
        return NULL;
    }
    if (function_matches_count > 1) {
        log_error_ast(ast, "to many functions match with the given parameter types");
        for (u64 i = 0; i < function_matches_count; i++) {
            Variable* function = function_matches[i];
            log_info_ast(function->ast, "could of meant");
        }
        return NULL;
    }

    Variable* function_var = function_matches[0];
    massert(function_var->type.kind == type_function, str("expected function"));
    Function* function = function_var->compile_time_value.function;
    return function;
}

Expression sem_function_call(Function* function, Expression* parameters, u64 parameter_count, Ast* ast) {
    for (u64 i = 0; i < parameter_count; i++) {
        Expression* expr = &parameters[i];
        Type* parameter_type = &function->function_type.function.parameter_types[i];
        if (!sem_type_equal_without_allocator(parameter_type, &expr->type)) {
            Expression as_function_type = sem_implicit_cast_without_allocator(expr, parameter_type);
            if (as_function_type.kind == expression_invalid) return (Expression){0};
            *expr = as_function_type;
        }
    }

    Function_Implementation_Parameter* implementation_parameters = cap_alloc(parameter_count * sizeof(Function_Implementation_Parameter));
    for (u64 i = 0; i < parameter_count; i++) {
        Function_Implementation_Parameter implementation_parameter = {0};
        implementation_parameter.type = function->function_type.function.parameter_types[i];
        Expression* expr = &parameters[i];
        if (expr->type.kind == type_type || expr->type.kind == type_function) {
            implementation_parameter.compile_time_value = expr->compile_time_value;
        } else {
            implementation_parameter.compile_time_value.has_value = false;
        }
        implementation_parameters[i] = implementation_parameter;
    }

    for (u64 i = 0; i < function->implementations_count; i++) {
        Function_Implementation* implementation = function->implementations[i];
        massert(implementation->parameter_count == parameter_count, str("expected same amount of parameters"));
        bool is_match = true;
        for (u64 j = 0; j < parameter_count; j++) {
            Function_Implementation_Parameter* implementation_parameter = &implementation->parameters[j];
            Function_Implementation_Parameter* function_parameter = &implementation_parameters[j];
            massert(sem_type_equal_without_allocator(&implementation_parameter->type, &function_parameter->type), str("expected same parameter types"));
            if (implementation_parameter->type.kind == type_type) {
                Type* underlying_type = implementation_parameter->compile_time_value.underlying_type;
                Type* function_underlying_type = function_parameter->compile_time_value.underlying_type;
                if (!sem_type_equal_without_allocator(underlying_type, function_underlying_type)) {
                    is_match = false;
                    break;
                }
            } else if (implementation_parameter->type.kind == type_function) {
                Function* implementation_function = implementation_parameter->compile_time_value.function;
                Function* function_function = function_parameter->compile_time_value.function;
                if (implementation_function != function_function) {
                    is_match = false;
                    break;
                }
            }
        }
        if (is_match) {
            Expression function_call_expr = {0};
            function_call_expr.kind = expression_function_call;
            Type return_type = sem_type_new_allocator_ids(&implementation->return_type);
            function_call_expr.type = return_type;
            function_call_expr.ast = ast;
            function_call_expr.function_call.parameters = parameters;
            function_call_expr.function_call.parameter_count = parameter_count;
            function_call_expr.function_call.implementation = implementation;
            return function_call_expr;
        }
    }

    Function_Implementation* implementation = cap_alloc(sizeof(Function_Implementation));
    implementation->parameter_count = parameter_count;
    implementation->parameters = implementation_parameters;
    implementation->body.parent = cap_context.scope;

    // temporaryly set namespace while we are building the implementation
    u64 last_namespace_we_are_in = cap_context.namespace_we_are_in;
    Scope* last_scope = cap_context.scope;
    Function_Implementation* last_function_being_built = cap_context.function_being_built;
    cap_context.function_being_built = implementation;
    cap_context.namespace_we_are_in = function->namespace_id;
    cap_context.scope = &implementation->body;

    for (u64 i = 0; i < parameter_count; i++) {
        Function_Implementation_Parameter* implementation_parameter = &implementation->parameters[i];
        Ast* function_ast = function->ast;
        massert(function_ast->kind == ast_function_declaration, str("expected function declaration"));
        Ast* parameter_ast = &function_ast->function_declaration.parameters[i];
        String parameter_name = parameter_ast->function_declaration_parameters.name;
        Type var_type = sem_type_new_allocator_ids(&implementation_parameter->type);
        Variable* var = sem_add_variable(parameter_name, var_type, parameter_ast, implementation_parameter->compile_time_value);
        var->compile_time_value = implementation_parameter->compile_time_value;
    }

    Type* func_return_type = &function->function_type.function.return_types[0];
    Type return_type = sem_type_new_allocator_ids(func_return_type);
    implementation->return_type = return_type;

    Ast* function_ast = function->ast;
    massert(function_ast->kind == ast_function_declaration, str("expected function declaration"));
    Ast* body_ast = function_ast->function_declaration.body;
    massert(body_ast->kind == ast_function_scope, str("expected function body to be a scope"));

    sem_scope_parse_statements(body_ast, &implementation->body);

    ptr_append(function->implementations, function->implementations_count, function->implementations_capacity, implementation);

    cap_context.namespace_we_are_in = last_namespace_we_are_in;
    cap_context.scope = last_scope;
    cap_context.function_being_built = last_function_being_built;

    Expression function_call_expr = {0};
    function_call_expr.kind = expression_function_call;
    function_call_expr.type = sem_type_new_allocator_ids(&implementation->return_type);
    function_call_expr.ast = ast;
    function_call_expr.function_call.parameters = parameters;
    function_call_expr.function_call.parameter_count = parameter_count;
    function_call_expr.function_call.implementation = implementation;
    if (function_call_expr.type.kind == type_type) {
    } else if (function_call_expr.type.kind == type_function) {
    }
    return function_call_expr;
}

void sem_scope_parse_statements(Ast* ast, Scope* scope) {
    Scope* last_scope = cap_context.scope;
    cap_context.scope = scope;

    massert(ast->kind == ast_function_scope, str("expected ast_scope"));
    for (u64 i = 0; i < ast->scope.statements_count; i++) {
        Ast* statement_ast = &ast->scope.statements[i];
        Statement statement = sem_statement_parse(statement_ast);
        if (statement.kind == statement_invalid) continue;
        ptr_append(scope->statements, scope->statements_count, scope->statements_capacity, statement);
    }

    cap_context.scope = last_scope;
}

Statement sem_statement_expression(Ast* ast) {
    massert(ast->kind == ast_nil_biop, str("expected ast_nil_biop"));
    Statement statement = {0};
    statement.kind = statement_expression;
    statement.ast = ast;
    Expression expression = sem_expression_parse(ast);
    if (expression.kind == expression_invalid) return (Statement){0};
    statement.expression.expression = expression;
    return statement;
}

bool sem_assign_expression(Expression* assignee, Expression* value) {
    if (assignee->kind == expression_variable_declaration) {
        Type* assignee_type = &assignee->variable_declaration.type;
        *value = sem_implicit_cast(value, assignee_type);
        if (value->kind == expression_invalid) return false;
        Variable* variable = sem_add_variable(assignee->variable_declaration.name, *assignee_type, assignee->ast, value->compile_time_value);
    } else {
        if (assignee->type.kind == type_type) {
            log_error_ast(assignee->ast, "can't assign to a type");
            return false;
        } else if (assignee->type.kind == type_function) {
            log_error_ast(assignee->ast, "can't assign to a function");
            return false;
        } else if (assignee->type.kind != type_reference) {
            log_error_ast(assignee->ast, "expected assignable value");
            return false;
        }
        Type assignee_type = sem_type_dereference(&assignee->type);
        *value = sem_implicit_cast(value, &assignee_type);
        if (value->kind == expression_invalid) return false;
    }

    return true;
}

Statement sem_statement_assignment(Ast* ast) {
    massert(ast->kind == ast_assignment, str("expected ast_assignment"));
    Statement statement = {0};
    statement.kind = statement_assignment;
    statement.ast = ast;
    u64 assignees_count = ast->assignment.assignees_count;
    u64 values_count = ast->assignment.values_count;
    if (assignees_count != values_count) {
        log_error_ast(ast, "expected same amount of assignees and values");
        return (Statement){0};
    }

    Expression* assignees = cap_alloc(assignees_count * sizeof(Expression));
    Expression* values = cap_alloc(values_count * sizeof(Expression));
    for (u64 i = 0; i < assignees_count; i++) {
        Ast* assignee = &ast->assignment.assignees[i];
        Expression assignee_expression = sem_expression_parse_with_variable_declaration(assignee);
        if (assignee_expression.kind == expression_invalid) return (Statement){0};
        assignees[i] = assignee_expression;
    }
    for (u64 i = 0; i < values_count; i++) {
        Ast* value = &ast->assignment.values[i];
        Expression value_expression = sem_expression_parse(value);
        if (value_expression.kind == expression_invalid) return (Statement){0};
        values[i] = value_expression;
    }

    for (u64 i = 0; i < assignees_count; i++) {
        Expression* assignee = &assignees[i];
        Expression* value = &values[i];
        if (!sem_assign_expression(assignee, value)) return (Statement){0};
    }

    statement.assignment.assignees = assignees;
    statement.assignment.values = values;
    statement.assignment.count = assignees_count;
    return statement;
}

Statement sem_statement_return(Ast* ast) {
    massert(ast->kind == ast_return, str("expected ast_return"));
    Statement statement = {0};
    statement.kind = statement_return;
    statement.ast = ast;
    Expression value = sem_expression_parse(ast->return_.value);
    if (value.kind == expression_invalid) return (Statement){0};
    Type* return_type = &cap_context.function_being_built->return_type;
    value = sem_implicit_cast(&value, return_type);
    if (value.kind == expression_invalid) return (Statement){0};
    statement.return_.value = value;
    return statement;
}

Statement sem_statement_parse(Ast* ast) {
    switch (ast->kind) {
        case ast_return:
            return sem_statement_return(ast);
        case ast_assignment:
            return sem_statement_assignment(ast);
        case ast_int:
        case ast_float:
        case ast_string:
        case ast_program:
        case ast_function_scope:
        case ast_variable:
        case ast_dereference:
        case ast_reference:
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
        case ast_nil_biop:
            return sem_statement_expression(ast);
        case ast_function_call:
        case ast_function_declaration:
        case ast_function_declaration_parameter:
        case ast_top_level:
        case ast_include:
        case ast_invalid: {
            log_error_ast(ast, "unexpected ast kind in statement parse");
            return (Statement){0};
        }
    }
}

Variable** __sem_find_functions_with_name_and_namespace(String name, String* namespaces, u64 namespaces_count, u64* out_count, Scope* scope) {
    u64 functions_count = 0;
    u64 functions_capacity = 8;
    Variable** functions = cap_alloc(functions_capacity * sizeof(Variable*));
    for (u64 i = 0; i < scope->variables_count; i++) {
        Variable* variable = scope->variables[i];
        if (variable->type.kind != type_function) continue;
        if (string_equal(variable->name, name) && sem_variable_fits_namespace(variable, namespaces, namespaces_count)) {
            ptr_append(functions, functions_count, functions_capacity, variable);
        };
    }
    if (functions_count == 0 && scope->parent != NULL) {
        return __sem_find_functions_with_name_and_namespace(name, namespaces, namespaces_count, out_count, scope->parent);
    }
    *out_count = functions_count;
    return functions;
}

Variable** sem_find_functions_with_name_and_namespace(String name, String* namespaces, u64 namespaces_count, u64* out_count) {
    Scope* scope = cap_context.scope;
    return __sem_find_functions_with_name_and_namespace(name, namespaces, namespaces_count, out_count, scope);
}

void sem_complete_variable_type(Variable* variable) {
    u64 namespace_we_are_in = variable->namespace;
    Type* type = &variable->type;
    if (type->is_complete) return;
    Type_Kind kind = type->kind;
    switch (kind) {
        case type_function: {
            Type_Function* function = &type->function;
            Function* f = variable->compile_time_value.function;
            Ast* ast = f->ast;
            massert(ast->kind == ast_function_declaration, str("expected function declaration"));

            // handel visited
            for (u64 i = 0; i < cap_context.visited_in_typing_count; i++) {
                Variable* vis = cap_context.visited_in_typing[i];
                if (vis == variable) return;
            }
            cap_context.visited_in_typing[cap_context.visited_in_typing_count] = variable;
            cap_context.visited_in_typing_count += 1;

            u64 return_types_count = ast->function_declaration.return_types_count;
            Type* return_types = cap_alloc(return_types_count * sizeof(Type));
            for (u64 i = 0; i < return_types_count; i++) {
                Ast* return_type = &ast->function_declaration.return_types[i];
                Type return_type_complete = sem_type_parse(return_type);
                return_types[i] = return_type_complete;
            }

            u64 parameter_types_count = ast->function_declaration.parameters_count;
            Type* parameter_types = cap_alloc(parameter_types_count * sizeof(Type));
            for (u64 i = 0; i < parameter_types_count; i++) {
                Ast* parameter = &ast->function_declaration.parameters[i];
                Ast* parameter_type_ast = parameter->function_declaration_parameters.type;
                Type parameter_type_complete = sem_type_parse(parameter_type_ast);
                parameter_types[i] = parameter_type_complete;
            }

            Type function_type = sem_function_type(return_types, return_types_count, parameter_types, parameter_types_count, NO_ALLOCATOR_ID);
            f->function_type = function_type;
            variable->type = function_type;

            cap_context.visited_in_typing_count -= 1;
            Variable* vis = cap_context.visited_in_typing[cap_context.visited_in_typing_count];
            massert(vis == variable, str("variable should be in visited in typing"));
            break;
        }
        case type_type: {
            massert(false, str("type type should never be complete"));
            break;
        }
        case type_int_literal:
        case type_float_literal:
        case type_void:
        case type_bool:
        case type_float:
        case type_uint:
        case type_int:
        case type_invalid:
        case type_reference:
        case type_pointer: {
            mabort(str("type should allways be complete"));
            break;
        }
    }
}

void sem_complete_types_in_global_scope() {
    Scope* global_scope = &cap_context.global_scope;
    for (u64 i = 0; i < global_scope->variables_count; i++) {
        Variable* variable = global_scope->variables[i];
        sem_complete_variable_type(variable);
    }
}

Expression sem_expression_nil_biop_parse(Ast* ast) {
    massert(ast->kind == ast_nil_biop, str("expected ast_nil_biop"));
    Expression expr = {0};
    expr.kind = expression_variable_declaration;
    Expression lhs = sem_expression_parse(ast->biop.lhs);
    if (lhs.kind == expression_invalid) return (Expression){0};
    if (lhs.type.kind != type_type) {
        log_error_ast(ast, "expected type");
        return (Expression){0};
    }
    Ast* rhs = ast->biop.rhs;
    massert(rhs->kind == ast_variable, str("expected ast_variable"));
    if (rhs->variable.namespaces_count != 0) {
        log_error_ast(ast, "can't use namespaces in variable declaration");
        return (Expression){0};
    }
    String name = rhs->variable.name;
    Type var_type = *lhs.compile_time_value.underlying_type;
    if (var_type.kind == type_invalid) return (Expression){0};

    if (var_type.kind == type_type || var_type.kind == type_function) {
        expr.type = var_type;
    } else {
        expr.type = sem_type_reference(&var_type, ast);
    }
    expr.variable_declaration.name = name;
    expr.variable_declaration.type = var_type;
    expr.ast = ast;
    return expr;
}

Expression sem_expression_variable_parse(Ast* ast) {
    massert(ast->kind == ast_variable, str("expected ast_variable"));
    Expression expr = {0};
    expr.kind = expression_variable;
    String name = ast->variable.name;
    String* namespaces = ast->variable.namespaces;
    u64 namespaces_count = ast->variable.namespaces_count;
    Variable* variable = sem_find_variable(name, namespaces, namespaces_count, ast);
    if (variable == NULL) return (Expression){0};
    expr.variable.variable = variable;
    if (variable->type.kind == type_type || variable->type.kind == type_function || variable->type.kind == type_reference) {
        expr.type = variable->type;
        if (variable->type.kind == type_type) {
            expr.compile_time_value.underlying_type = cap_alloc(sizeof(Type));
            *expr.compile_time_value.underlying_type = sem_type_new_allocator_ids(&variable->type);
        } else {
            expr.compile_time_value = variable->compile_time_value;
        }
    } else {
        expr.type = sem_type_reference(&variable->type, ast);
        if (expr.type.kind == type_invalid) return (Expression){0};
        expr.compile_time_value = variable->compile_time_value;
    }
    expr.ast = ast;
    return expr;
}

Expression sem_expression_int_parse(Ast* ast) {
    massert(ast->kind == ast_int, str("expected ast_int"));
    Expression expr = {0};
    expr.kind = expression_int;
    expr.int_value.value = ast->int_value.value;
    expr.type = sem_type_int_literal();
    expr.ast = ast;
    expr.compile_time_value.has_value = true;
    expr.compile_time_value.i64_value = ast->int_value.value;
    return expr;
}

Expression sem_expression_float_parse(Ast* ast) {
    massert(ast->kind == ast_float, str("expected ast_float"));
    Expression expr = {0};
    expr.kind = expression_float;
    expr.float_value.value = ast->float_value.value;
    expr.type = sem_type_float_literal();
    expr.ast = ast;
    expr.compile_time_value.has_value = true;
    expr.compile_time_value.f64_value = ast->float_value.value;
    return expr;
}

Expression sem_expression_reference(Ast* ast) {
    massert(ast->kind == ast_reference, str("expected ast_reference"));
    Expression value = sem_expression_parse(ast->reference.value);
    if (value.kind == expression_invalid) return (Expression){0};
    Expression expr = sem_reference(&value);
    if (expr.kind == expression_invalid) return (Expression){0};
    expr.ast = ast;
    return expr;
}

Expression sem_expression_dereference(Ast* ast) {
    massert(ast->kind == ast_dereference, str("expected ast_dereference"));
    Expression value = sem_expression_parse(ast->dereference.value);
    if (value.kind == expression_invalid) return (Expression){0};
    Expression expr = sem_dereference(&value);
    if (expr.kind == expression_invalid) return (Expression){0};
    expr.ast = ast;
    return expr;
}

Expression sem_expression_multiply(Ast* ast) {
    massert(ast->kind == ast_multiply, str("expected ast_multiply"));
    Expression lhs = sem_expression_parse(ast->biop.lhs);
    if (lhs.kind == expression_invalid) return (Expression){0};
    if (lhs.type.kind == type_type) {
        // rewrite the tree to be a nil_biop
        Ast deref_lhs = {0};
        deref_lhs.kind = ast_dereference;
        deref_lhs.dereference.value = ast->biop.lhs;

        ast->kind = ast_nil_biop;
        ast->biop.lhs = cap_alloc(sizeof(Ast));
        *ast->biop.lhs = deref_lhs;

        return sem_expression_nil_biop_parse(ast);
    }
    massert(false, str("not implemented"));
    return (Expression){0};
}

Expression sem_bitwise_and(Ast* ast) {
    massert(ast->kind == ast_bitwise_and, str("expected ast_bitwise_and"));
    Expression lhs = sem_expression_parse(ast->biop.lhs);
    if (lhs.kind == expression_invalid) return (Expression){0};
    if (lhs.type.kind == type_type) {
        // rewrite the tree to be a nil_biop
        Ast deref_lhs = {0};
        deref_lhs.kind = ast_dereference;
        deref_lhs.dereference.value = ast->biop.lhs;

        ast->kind = ast_nil_biop;
        ast->biop.lhs = cap_alloc(sizeof(Ast));
        *ast->biop.lhs = deref_lhs;

        return sem_expression_nil_biop_parse(ast);
    }
    massert(false, str("not implemented"));
    return (Expression){0};
}

Expression sem_expression_function_call_parse(Ast* ast) {
    massert(ast->kind == ast_function_call, str("expected ast_function_call"));
    Ast* parameters_ast = ast->function_call.parameters;
    u64 parameter_count = ast->function_call.parameters_count;

    Expression* parameters = cap_alloc(parameter_count * sizeof(Expression));
    for (u64 i = 0; i < parameter_count; i++) {
        Ast* parameter_ast = &parameters_ast[i];
        Expression parameter = sem_expression_parse(parameter_ast);
        if (parameter.kind == expression_invalid) return (Expression){0};
        parameters[i] = parameter;
    }

    Ast* function_name_ast = ast->function_call.function_variable;
    massert(function_name_ast->kind == ast_variable, str("expected ast_variable"));
    String function_name = function_name_ast->variable.name;
    String* namespaces = function_name_ast->variable.namespaces;
    u64 namespaces_count = function_name_ast->variable.namespaces_count;

    Type* parameters_types = cap_alloc(parameter_count * sizeof(Type));
    for (u64 i = 0; i < parameter_count; i++) {
        Type* parameter_type = &parameters_types[i];
        parameters_types[i] = parameters[i].type;
    }

    Function* function = sem_find_function(function_name, namespaces, namespaces_count, parameters_types, parameter_count, ast);

    return sem_function_call(function, parameters, parameter_count, ast);
}

Expression sem_expression_parse_with_variable_declaration(Ast* ast) {
    switch (ast->kind) {
        case ast_int: {
            return sem_expression_int_parse(ast);
        }
        case ast_float: {
            return sem_expression_float_parse(ast);
        }
        case ast_variable: {
            return sem_expression_variable_parse(ast);
        }
        case ast_nil_biop: {
            return sem_expression_nil_biop_parse(ast);
        }
        case ast_string: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_function_call: {
            return sem_expression_function_call_parse(ast);
        }
        case ast_reference: {
            return sem_expression_reference(ast);
        }
        case ast_dereference: {
            return sem_expression_dereference(ast);
        }
        case ast_subtract: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_multiply: {
            return sem_expression_multiply(ast);
        }
        case ast_divide: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_modulo: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_greater: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_less: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_greater_equal: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_less_equal: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_logical_and: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_bitwise_and: {
            return sem_bitwise_and(ast);
        }
        case ast_logical_or: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_bitwise_or: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_shift_left: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_shift_right: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_add: {
            massert(false, str("not implemented"));
            return (Expression){0};
        }
        case ast_return:
        case ast_assignment:
        case ast_program:
        case ast_function_scope:
        case ast_top_level:
        case ast_include:
        case ast_function_declaration:
        case ast_function_declaration_parameter:
        case ast_invalid: {
            log_error_ast(ast, "unexpected ast kind in expression parse");
            return (Expression){0};
        }
    }
}

Expression sem_dereference(Expression* expr) {
    if (expr->type.kind == type_function) {
        log_error_ast(expr->ast, "can't dereference a function");
        return (Expression){0};
    }
    Expression deref = {0};
    deref.kind = expression_dereference;
    deref.ast = expr->ast;
    deref.dereference.expr = cap_alloc(sizeof(Expression));
    *deref.dereference.expr = *expr;
    if (expr->type.kind == type_type) {
        deref.type = sem_type_type();
        Type* old_underlying_type = expr->compile_time_value.underlying_type;
        Type new_underlying_type = sem_type_pointer(old_underlying_type, expr->ast);
        if (new_underlying_type.kind == type_invalid) return (Expression){0};
        deref.compile_time_value.underlying_type = cap_alloc(sizeof(Type));
        *deref.compile_time_value.underlying_type = new_underlying_type;
    } else {
        deref.type = sem_type_dereference(&expr->type);
        if (deref.type.kind == type_invalid) return (Expression){0};
    }
    return deref;
}

Expression sem_reference(Expression* expr) {
    if (expr->type.kind == type_function) {
        log_error_ast(expr->ast, "can't reference a function");
        return (Expression){0};
    }
    Expression ref = {0};
    ref.kind = expression_reference;
    ref.ast = expr->ast;
    ref.reference.expr = cap_alloc(sizeof(Expression));
    *ref.reference.expr = *expr;
    if (expr->type.kind == type_type) {
        ref.type = sem_type_type();
        Type* old_underlying_type = expr->compile_time_value.underlying_type;
        Type new_underlying_type = sem_type_reference(old_underlying_type, expr->ast);
        if (new_underlying_type.kind == type_invalid) return (Expression){0};
        ref.compile_time_value.underlying_type = cap_alloc(sizeof(Type));
        *ref.compile_time_value.underlying_type = new_underlying_type;
    } else {
        ref.type = sem_type_reference(&expr->type, expr->ast);
        if (ref.type.kind == type_invalid) return (Expression){0};
        ref.compile_time_value = expr->compile_time_value;
    }
    return ref;
}

bool sem_can_implicit_cast(Expression* expr, Type* type) {
    Type expr_type = expr->type;
    if (type->kind != type_reference && expr->type.kind == type_reference) {
        expr_type = sem_type_dereference(&expr_type);
    }
    if (sem_type_equal_without_allocator(&expr_type, type)) return true;
    if (expr_type.kind == type_int && type->kind == type_int) {
        u64 expr_bits = expr_type.int_.bits;
        u64 type_bits = type->int_.bits;
        if (expr_bits <= type_bits) return true;
        else return false;
    }
    if (expr_type.kind == type_uint && type->kind == type_uint) {
        u64 expr_bits = expr_type.uint.bits;
        u64 type_bits = type->uint.bits;
        if (expr_bits <= type_bits) return true;
        else return false;
    }
    if (expr_type.kind == type_bool && type->kind == type_bool) return true;
    if (expr_type.kind == type_float && type->kind == type_float) return true;
    if (expr_type.kind == type_int_literal && type->kind == type_int) return true;
    if (expr_type.kind == type_int_literal && type->kind == type_uint) return true;
    if (expr_type.kind == type_int_literal && type->kind == type_float) return true;
    if (expr_type.kind == type_float_literal && type->kind == type_float) return true;
    return false;
}

Expression _sem_cast(Expression* expr_in, Type* type, bool connect_allocator) {
    Expression expr = *expr_in;
    if (type->kind != type_reference && expr.type.kind == type_reference) {
        expr = sem_dereference(expr_in);
        if (expr.kind == expression_invalid) return (Expression){0};
    }

    Expression cast = {0};
    cast.kind = expression_cast;
    cast.ast = expr.ast;
    cast.cast.expr = cap_alloc(sizeof(Expression));
    *cast.cast.expr = expr;

    if (connect_allocator) {
        Type et = expr.type;
        Type tt = *type;
        while (et.kind == type_pointer && tt.kind == type_pointer) {
            // TODO: checks to make sure this is valid
            sem_connect_allocator_ids(et.pointer.underlying_type->allocator_id, tt.pointer.underlying_type->allocator_id);
            et = sem_type_underlying_type(&et);
            tt = sem_type_underlying_type(&tt);
        }
        massert(et.kind != type_pointer && tt.kind != type_pointer, str("expected no pointers"));
        sem_connect_allocator_ids(et.allocator_id, tt.allocator_id);
    }

    cast.type = *type;
    return cast;
}

Expression sem_cast_without_allocator(Expression* expr, Type* type) {
    return _sem_cast(expr, type, false);
}

Expression sem_cast(Expression* expr, Type* type) {
    return _sem_cast(expr, type, true);
}

Expression _sem_implicit_cast(Expression* expr, Type* type, bool connect_allocator) {
    if (sem_type_equal(type, &expr->type)) return *expr;
    if (sem_can_implicit_cast(expr, type)) {
        Expression cast = _sem_cast(expr, type, connect_allocator);
        if (cast.kind == expression_invalid) return (Expression){0};
        return cast;
    }
    String expr_str = sem_type_to_string(&expr->type);
    String type_str = sem_type_to_string(type);
    log_error_ast(expr->ast, "cannot implicitly cast %.*s to %.*s", str_info(expr_str), str_info(type_str));
    return (Expression){0};
}

Expression sem_implicit_cast_without_allocator(Expression* expr, Type* type) {
    return _sem_implicit_cast(expr, type, false);
}

Expression sem_implicit_cast(Expression* expr, Type* type) {
    return _sem_implicit_cast(expr, type, true);
}

Expression sem_expression_parse(Ast* ast) {
    Expression expr = sem_expression_parse_with_variable_declaration(ast);
    if (expr.kind == expression_variable_declaration) {
        Variable* variable = expr.variable_declaration.variable;
        *variable = (Variable){0};
        variable->name = str("$$$$$INVALID$$$$$");
        variable->namespace = UINT64_MAX;
        log_error_ast(ast, "variable declarations are not allowed in expressions");
        return (Expression){0};
    }
    return expr;
}

Program sem_program_parse(Ast* ast) {
    Program program = {0};
    program.name = ast->program.name;
    Type return_type = sem_int_type(32);
    Type function_type = sem_function_type(&return_type, 1, NULL, 0, NO_ALLOCATOR_ID);
    Function function = sem_create_function(function_type, NULL, ast);
    program.function = function;

    Function_Implementation* implmentation = cap_alloc(sizeof(Function_Implementation));
    implmentation->parameter_count = 0;
    implmentation->parameters = NULL;
    implmentation->body.parent = function.scope_created_in;
    implmentation->return_type = sem_int_type(32);
    ptr_append(function.implementations, function.implementations_count, function.implementations_capacity, implmentation);

    Scope* last_scope = cap_context.scope;
    Function_Implementation* last_function_being_built = cap_context.function_being_built;
    u64 last_namespace_we_are_in = cap_context.namespace_we_are_in;
    cap_context.function_being_built = implmentation;
    cap_context.namespace_we_are_in = function.namespace_id;

    sem_scope_parse_statements(ast->program.body, &implmentation->body);

    cap_context.function_being_built = last_function_being_built;
    cap_context.namespace_we_are_in = last_namespace_we_are_in;

    return program;
}
