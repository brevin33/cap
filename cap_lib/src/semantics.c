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
            snprintf(buffer, 2048, "%.*s*", str_info(underlying_type_str));
            u64 length = strlen(buffer);
            char* ptr = cap_alloc(length + 1);
            memcpy(ptr, buffer, length);
            return string_create(ptr, length);
        }
        case type_reference: {
            String underlying_type_str = sem_type_to_string(type->reference.underlying_type);
            String str = {0};
            char buffer[2048];
            snprintf(buffer, 2048, "%.*s&", str_info(underlying_type_str));
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
        case type_uint: {
            i64 bits = type->uint.bits;
            String number_str = string_int(bits);
            String str = str("u");
            str = string_append(str, number_str);
            return str;
        }
        case type_multiple_value: {
            Type_Multiple_Value* multi_value = &type->multiple_value;
            String str = str("multi_value(");
            for (u64 i = 0; i < multi_value->types_count; i++) {
                Type* type = &multi_value->types[i];
                String type_str = sem_type_to_string(type);
                str = string_append(str, type_str);
                if (i != multi_value->types_count - 1) str = string_append(str, str(", "));
            }
            str = string_append(str, str(")"));
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

Type sem_float_type(i64 bits) {
    Type type = {0};
    type.kind = type_float;
    type.float_.bits = bits;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_type_reference(Type* underlying_type, Ast* ast_for_error) {
    if (underlying_type->kind == type_reference) {
        log_error_ast(ast_for_error, "can't have a reference to a reference");
        return (Type){0};
    }
    Type type = {0};
    type.kind = type_reference;
    type.reference.underlying_type = cap_alloc(sizeof(Type));
    *type.reference.underlying_type = *underlying_type;
    type.allocator_id = sem_get_new_allocator_id();
    return type;
}

Type sem_type_pointer(Type* underlying_type, Ast* ast_for_error) {
    if (underlying_type->kind == type_reference) {
        log_error_ast(ast_for_error, "can't have a pointer to a reference");
        return (Type){0};
    }
    Type type = {0};
    type.kind = type_pointer;
    type.pointer.underlying_type = cap_alloc(sizeof(Type));
    *type.pointer.underlying_type = *underlying_type;
    type.allocator_id = sem_get_new_allocator_id();
    return type;
}

Type sem_type_int_literal() {
    Type type = {0};
    type.kind = type_int_literal;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_type_float_literal() {
    Type type = {0};
    type.kind = type_float_literal;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_type_invalid() {
    Type type = {0};
    type.kind = type_invalid;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_type_multiple_value(Type* types, u64 types_count) {
    Type type = {0};
    type.kind = type_multiple_value;
    type.multiple_value.types = types;
    type.multiple_value.types_count = types_count;
    type.allocator_id = NO_ALLOCATOR_ID;
    return type;
}

Type sem_type_dereference(Type* type) {
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
        case type_multiple_value: {
            Type_Multiple_Value* multi_value = &type->multiple_value;
            Type_Multiple_Value new_multi_value = *multi_value;
            new_multi_value.types = cap_alloc(multi_value->types_count * sizeof(Type));
            new_multi_value.types_count = multi_value->types_count;
            for (u64 i = 0; i < multi_value->types_count; i++) {
                Type* type = &multi_value->types[i];
                Type new_type = sem_type_new_allocator_ids(type);
                new_multi_value.types[i] = new_type;
            }
            Type new_type = *type;
            new_type.kind = type_multiple_value;
            new_type.multiple_value = new_multi_value;
            new_type.allocator_id = NO_ALLOCATOR_ID;
            return new_type;
        }
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
        case type_multiple_value: {
            Type_Multiple_Value* multi_value_a = &type_a->multiple_value;
            Type_Multiple_Value* multi_value_b = &type_b->multiple_value;
            if (multi_value_a->types_count != multi_value_b->types_count) return false;
            for (u64 i = 0; i < multi_value_a->types_count; i++) {
                Type* type_a = &multi_value_a->types[i];
                Type* type_b = &multi_value_b->types[i];
                if (!sem_type_allocator_equal(type_a, type_b)) return false;
            }
            return true;
        }
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
        case type_multiple_value: {
            Type_Multiple_Value* multi_value_a = &type_a->multiple_value;
            Type_Multiple_Value* multi_value_b = &type_b->multiple_value;
            if (multi_value_a->types_count != multi_value_b->types_count) return false;
            for (u64 i = 0; i < multi_value_a->types_count; i++) {
                Type* type_a = &multi_value_a->types[i];
                Type* type_b = &multi_value_b->types[i];
                if (!sem_type_equal_without_allocator(type_a, type_b)) return false;
            }
            return true;
        }
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

void _sem_get_dependent_variable_in_expression(Expression* expression, Expression*** out_dependent_expressions, u64* out_count, u64* out_capacity,
                                               Function_Implementation*** out_implementations, u64* out_implementations_count,
                                               u64* out_implementations_capacity) {
    switch (expression->kind) {
        case expression_passthrough: {
            Expression* expr = expression->passthrough.expr;
            _sem_get_dependent_variable_in_expression(expr, out_dependent_expressions, out_count, out_capacity, out_implementations, out_implementations_count,
                                                      out_implementations_capacity);
            return;
        }
        case expression_multiple_values_access: {
            Expression* multiple_values_value = expression->multiple_values_access.multiple_values_value;
            _sem_get_dependent_variable_in_expression(multiple_values_value, out_dependent_expressions, out_count, out_capacity, out_implementations,
                                                      out_implementations_count, out_implementations_capacity);
            return;
        }
        case expression_variable: {
            ptr_append(*out_dependent_expressions, *out_count, *out_capacity, expression);
            return;
        }
        case expression_dereference: {
            _sem_get_dependent_variable_in_expression(expression->dereference.expr, out_dependent_expressions, out_count, out_capacity, out_implementations,
                                                      out_implementations_count, out_implementations_capacity);
            return;
        }
        case expression_cast: {
            _sem_get_dependent_variable_in_expression(expression->cast.expr, out_dependent_expressions, out_count, out_capacity, out_implementations,
                                                      out_implementations_count, out_implementations_capacity);
            return;
        }
        case expression_reference: {
            _sem_get_dependent_variable_in_expression(expression->reference.expr, out_dependent_expressions, out_count, out_capacity, out_implementations,
                                                      out_implementations_count, out_implementations_capacity);
            return;
        }
        case expression_function_call: {
            Function_Implementation* implementation = expression->function_call.implementation;
            ptr_append(*out_implementations, *out_implementations_count, *out_implementations_capacity, implementation);
            for (u64 i = 0; i < expression->function_call.parameter_count; i++) {
                Expression* parameter = &expression->function_call.parameters[i];
                _sem_get_dependent_variable_in_expression(parameter, out_dependent_expressions, out_count, out_capacity, out_implementations,
                                                          out_implementations_count, out_implementations_capacity);
            }
            return;
        }
        case expression_incomplete:
        case expression_invalid:
        case expression_int:
        case expression_float:
        case expression_variable_declaration:
            return;
    }
}

void* _sem_evaluate_expression_trivially(Expression* expression) {
    switch (expression->kind) {
        case expression_dereference: {
            Expression* expr = expression->dereference.expr;
            void** value_ptr = _sem_evaluate_expression_trivially(expr);
            if (value_ptr == NULL) return NULL;
            else if (expr->type.kind == type_reference) return *value_ptr;
            else if (expr->type.kind == type_pointer) return value_ptr;
            else if (expr->type.kind == type_type) {
                Type** value_type_ptr = (Type**)value_ptr;
                Type ptr_type = sem_type_pointer(*value_type_ptr, expression->ast);
                Type* type_ptr = cap_alloc(sizeof(Type));
                *type_ptr = ptr_type;
                Type** type_ptr_ptr = cap_alloc(sizeof(Type*));
                *type_ptr_ptr = type_ptr;
                return type_ptr_ptr;
            } else return NULL;
        }
        case expression_variable: {
            Variable* variable = expression->variable.variable;
            massert(variable->know_compile_time_value, str("expected compile time value"));
            void* value = variable->compile_time_value;
            void** value_ptr = cap_alloc(sizeof(void*));
            *value_ptr = value;
            return value_ptr;
        }
        case expression_passthrough: {
            return sem_evaluate_expression(expression->passthrough.expr);
        }
        case expression_cast: {
            Expression* underlying_expr = expression->cast.expr;
            Type* underlying_type = &underlying_expr->type;
            Type* current_type = &expression->type;
            if (sem_type_equal_without_allocator(underlying_type, current_type)) {
                return sem_evaluate_expression(underlying_expr);
            }
            return NULL;
        }
        default:
            return NULL;
    }
}

bool sem_comile_time_value_is_equal(Type* type, void* value_a, void* value_b) {
    if (value_a == NULL || value_b == NULL) return false;
    switch (type->kind) {
        case type_int_literal: {
            i64 a = *(i64*)value_a;
            i64 b = *(i64*)value_b;
            return a == b;
        }
        case type_float_literal: {
            f64 a = *(f64*)value_a;
            f64 b = *(f64*)value_b;
            return a == b;
        }
        case type_int: {
            i64 bits = type->int_.bits;
            i64 bytes = bits / 8;
            char a[bytes];
            char b[bytes];
            memcpy(a, value_a, bytes);
            memcpy(b, value_b, bytes);
            return memcmp(a, b, bytes) == 0;
        }
        case type_uint: {
            i64 bits = type->uint.bits;
            i64 bytes = bits / 8;
            char a[bytes];
            char b[bytes];
            memcpy(a, value_a, bytes);
            memcpy(b, value_b, bytes);
            return memcmp(a, b, bytes) == 0;
        }
        case type_float: {
            i64 bits = type->float_.bits;
            i64 bytes = bits / 8;
            char a[bytes];
            char b[bytes];
            memcpy(a, value_a, bytes);
            memcpy(b, value_b, bytes);
            return memcmp(a, b, bytes) == 0;
        }
        case type_type: {
            Type** a = (Type**)value_a;
            Type** b = (Type**)value_b;
            Type* a_type = *a;
            Type* b_type = *b;
            return sem_type_equal_without_allocator(a_type, b_type);
        }
        case type_reference: {
            Type* underlying_type = type->reference.underlying_type;
            void* a = *(void**)value_a;
            void* b = *(void**)value_b;
            return sem_comile_time_value_is_equal(underlying_type, a, b);
        }
        case type_function: {
            Function* a_function = *(Function**)value_a;
            Function* b_function = *(Function**)value_b;
            return a_function == b_function;
        }
        case type_pointer:
        case type_void:
        case type_invalid:
        case type_multiple_value: {
            return false;
        }
    }
}

void* _sem_evaluate_expression(Expression* expression) {
    Expression** variable_exprs = NULL;
    u64 variable_exprs_capacity = 0;
    u64 variable_exprs_count = 0;
    Function_Implementation** implementations = NULL;
    u64 implementations_capacity = 0;
    u64 implementations_count = 0;
    _sem_get_dependent_variable_in_expression(expression, &variable_exprs, &variable_exprs_count, &variable_exprs_capacity, &implementations,
                                              &implementations_count, &implementations_capacity);
    for (u64 i = 0; i < implementations_count; i++) {
        Function_Implementation* implementation = implementations[i];
        sem_complete_implementation(implementation);
    }

    for (u64 i = 0; i < variable_exprs_count; i++) {
        Expression* variable_expr = variable_exprs[i];
        massert(variable_expr->kind == expression_variable, str("expected variable expression"));
        Variable* variable = variable_expr->variable.variable;
        if (variable->lost_constant_at_ast) {
            log_error_ast(variable_expr->ast, "can't evaluate at compile time variable that isn't constant");
            log_info_ast(variable->lost_constant_at_ast, "where variable lost its constantness");
            return NULL;
        }
        if (variable->know_compile_time_value) {
            continue;
        }
        if (!variable->initial_value) {
            log_error_ast(variable_expr->ast, "variable has no initial value so can't be evaluated at compile time");
            log_info_ast(variable->ast, "variable declared here");
            return NULL;
        }
        void* value = sem_evaluate_expression(variable->initial_value);
        if (value == NULL) {
            log_error_ast(variable_expr->ast, "can't evaluate compile time");
            return NULL;
        }
        variable->compile_time_value = value;
        variable->know_compile_time_value = true;
    }
    void* value = _sem_evaluate_expression_trivially(expression);
    if (value != NULL) return value;
    return llvm_evaluate_expression(expression);
}

void* sem_evaluate_expression(Expression* expression) {
    void* value = _sem_evaluate_expression(expression);
    if (value == NULL) return NULL;
    if (expression->type.kind == type_type) {
        Type** type_ptr = value;
        Type* type = *type_ptr;
        Type type_new = sem_type_new_allocator_ids(type);
        Type* new_type = cap_alloc(sizeof(Type));
        *new_type = type_new;
        Type** new_type_ptr = cap_alloc(sizeof(Type*));
        *new_type_ptr = new_type;
        return new_type_ptr;
    }
    return value;
}

Type sem_type_parse(Ast* ast) {
    Expression expr = sem_expression_parse(ast);
    expr = sem_get_value_if(&expr);
    if (expr.kind == expression_invalid) return (Type){0};
    if (expr.type.kind != type_type) {
        log_error_ast(ast, "expected type");
        return (Type){0};
    }
    Type** type_ptr = sem_evaluate_expression(&expr);
    if (type_ptr == NULL) return (Type){0};
    Type* type = *type_ptr;
    return *type;
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
    Variable* variable = __sem_find_variable(name, namespaces, namespaces_count, ast_for_error, cap_context.scope);
    if (variable == NULL) return NULL;
    sem_complete_variable_type(variable);
    if (!variable->is_type_complete) return NULL;
    return variable;
}

Variable* sem_add_variable(String name, Type type, Ast* ast) {
    Variable variable = {0};
    variable.namespace = cap_context.namespace_we_are_in;
    variable.name = name;
    variable.type = type;
    variable.ast = ast;
    variable.is_type_complete = true;
    variable.lost_constant_at_ast = NULL;

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

Function* sem_find_function(String name, String* namespaces, u64 namespaces_count, Type* _parameters, u64 parameter_count, Ast* ast) {
    // dereference all parameters
    Type* parameters = cap_alloc(parameter_count * sizeof(Type));
    for (u64 i = 0; i < parameter_count; i++) {
        Type* parameter_type = &_parameters[i];
        if (parameter_type->kind == type_reference) {
            parameters[i] = sem_type_dereference(parameter_type);
        } else {
            parameters[i] = *parameter_type;
        }
    }

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
        cap_context.log = true;
        if (!function->is_type_complete) continue;

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
    massert(function_var->know_compile_time_value, str("expected compile time value"));
    Function** function = function_var->compile_time_value;
    return *function;
}

Expression sem_multiple_values_access(Expression* multiple_values_value, u64 index, Ast* ast) {
    massert(multiple_values_value->type.kind == type_multiple_value, str("expected multiple values"));
    if (index >= multiple_values_value->type.multiple_value.types_count) {
        log_error_ast(ast, "index %llu is out of bounds for multiple values of size %llu", index, multiple_values_value->type.multiple_value.types_count);
        return (Expression){0};
    }
    Expression expr = {0};
    expr.kind = expression_multiple_values_access;
    expr.multiple_values_access.multiple_values_value = multiple_values_value;
    expr.multiple_values_access.index = index;
    expr.ast = ast;
    expr.type = multiple_values_value->type.multiple_value.types[index];
    return expr;
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

    Function_Implementation* implementation = cap_alloc(sizeof(Function_Implementation));
    implementation->function = function;
    implementation->parameter_count = parameter_count;
    implementation->parameters = cap_alloc(parameter_count * sizeof(Variable*));
    implementation->body.parent = cap_context.scope;
    implementation->is_complete = false;

    // temporaryly set namespace while we are building the implementation
    u64 last_namespace_we_are_in = cap_context.namespace_we_are_in;
    Scope* last_scope = cap_context.scope;
    Function_Implementation* last_function_being_built = cap_context.function_being_built;
    cap_context.function_being_built = implementation;
    cap_context.namespace_we_are_in = function->namespace_id;
    cap_context.scope = &implementation->body;

    for (u64 i = 0; i < parameter_count; i++) {
        Expression* parameter_expression = &parameters[i];
        Type* parameter_type = &parameter_expression->type;
        Ast* function_ast = function->ast;
        massert(function_ast->kind == ast_function_declaration, str("expected function declaration"));
        Ast* parameter_ast = &function_ast->function_declaration.parameters[i];
        String parameter_name = parameter_ast->function_declaration_parameters.name;
        Type var_type = sem_type_new_allocator_ids(parameter_type);
        Variable* var = sem_add_variable(parameter_name, var_type, parameter_ast);
        var->initial_value = parameter_expression;
        implementation->parameters[i] = var;
    }

    Type* func_return_type = &function->function_type.function.return_types[0];

    Type* return_types = cap_alloc(function->function_type.function.return_types_count * sizeof(Type));
    for (u64 i = 0; i < function->function_type.function.return_types_count; i++) {
        Type* return_type = &function->function_type.function.return_types[i];
        Type return_type_new = sem_type_new_allocator_ids(return_type);
        return_types[i] = return_type_new;
    }
    implementation->return_types = return_types;
    implementation->return_types_count = function->function_type.function.return_types_count;

    Ast* function_ast = function->ast;
    massert(function_ast->kind == ast_function_declaration, str("expected function declaration"));
    Ast* body_ast = function_ast->function_declaration.body;
    massert(body_ast->kind == ast_function_scope, str("expected function body to be a scope"));

    ptr_append(function->implementations, function->implementations_count, function->implementations_capacity, implementation);

    cap_context.namespace_we_are_in = last_namespace_we_are_in;
    cap_context.scope = last_scope;
    cap_context.function_being_built = last_function_being_built;

    Expression function_call_expr = {0};
    function_call_expr.kind = expression_function_call;

    Type* expr_return_types = cap_alloc(implementation->return_types_count * sizeof(Type));
    for (u64 i = 0; i < implementation->return_types_count; i++) {
        Type* return_type = &implementation->return_types[i];
        Type return_type_new = sem_type_new_allocator_ids(return_type);
        expr_return_types[i] = return_type_new;
    }
    if (implementation->return_types_count == 1) {
        function_call_expr.type = return_types[0];
    } else {
        function_call_expr.type = sem_type_multiple_value(return_types, implementation->return_types_count);
    }

    function_call_expr.ast = ast;
    function_call_expr.function_call.parameters = parameters;
    function_call_expr.function_call.parameter_count = parameter_count;
    function_call_expr.function_call.implementation = implementation;

    Expression passthrough_expr = sem_passthrough(&function_call_expr);
    Expression* function_call_expr_ptr = passthrough_expr.passthrough.expr;
    ptr_append(cap_context.expression_to_complete, cap_context.expression_to_complete_count, cap_context.expression_to_complete_capacity,
               function_call_expr_ptr);
    return passthrough_expr;
}

void sem_complete_expression(Expression* expression) {
    switch (expression->kind) {
        case expression_passthrough: {
            sem_complete_expression(expression->passthrough.expr);
            return;
        }
        case expression_function_call: {
            Function_Implementation* call_implementation = expression->function_call.implementation;
            if (!call_implementation->is_complete) {
                // try to see if a already made implementation works for this so we don't have to make a new one
                Function* function = call_implementation->function;
                for (u64 i = 0; i < function->implementations_count; i++) {
                    Function_Implementation* implementation = function->implementations[i];
                    if (implementation->is_complete) {
                        bool is_match = true;
                        for (u64 j = 0; j < implementation->parameter_count; j++) {
                            Variable* call_parameter = call_implementation->parameters[j];
                            Variable* parameter = implementation->parameters[j];
                            Type* parameter_type = &parameter->type;
                            Type* call_parameter_type = &call_parameter->type;
                            if (!sem_type_equal_without_allocator(parameter_type, call_parameter_type)) {
                                is_match = false;
                                break;
                            }
                            if (parameter->know_compile_time_value) {
                                void* call_parameter_value = sem_evaluate_expression(call_parameter->initial_value);
                                void* parameter_value = parameter->compile_time_value;
                                if (!sem_comile_time_value_is_equal(parameter_type, call_parameter_value, parameter_value)) {
                                    is_match = false;
                                    break;
                                }
                            }
                        }
                        if (is_match) {
                            expression->function_call.implementation = implementation;
                            call_implementation = implementation;
                            break;
                        }
                    }
                }
                sem_complete_implementation(call_implementation);
            }

            // TODO: validate that allocators are ok

            return;
        }
        default: {
            mabort(str("expression should always be complete"));
        }
    }
}

void sem_complete_implementation(Function_Implementation* implementation) {
    if (implementation->is_complete) return;
    implementation->is_complete = true;

    cap_context.implementation_to_complete_recursion_counter++;
    u64 max_recursion_depth = 256;
    if (cap_context.implementation_to_complete_recursion_counter > max_recursion_depth) {
        Function* function = implementation->function;
        Ast* ast = function->ast;
        log_error_ast(ast, "hit max compile time function implementation recursion depth %llu", max_recursion_depth);
        return;
    }

    Scope* last_scope = cap_context.scope;
    Function_Implementation* last_function_being_built = cap_context.function_being_built;
    u64 last_namespace_we_are_in = cap_context.namespace_we_are_in;
    cap_context.function_being_built = implementation;
    cap_context.namespace_we_are_in = implementation->function->namespace_id;
    cap_context.scope = &implementation->body;

    Ast* function_ast = implementation->function->ast;
    massert(function_ast->kind == ast_function_declaration, str("expected function declaration"));
    Ast* body_ast = function_ast->function_declaration.body;

    sem_scope_parse_statements(body_ast, &implementation->body);

    cap_context.scope = last_scope;
    cap_context.function_being_built = last_function_being_built;
    cap_context.namespace_we_are_in = last_namespace_we_are_in;

    cap_context.implementation_to_complete_recursion_counter--;
}

Function_Implementation* sem_prototype_implementation(Function* function, Expression* parameters, u64 parameter_count) {
    Function_Implementation* implementation = cap_alloc(sizeof(Function_Implementation));
    implementation->function = function;
    implementation->parameter_count = parameter_count;
    implementation->parameters = cap_alloc(parameter_count * sizeof(Variable*));
    implementation->body.parent = cap_context.scope;

    // temporaryly set namespace while we are building the implementation
    u64 last_namespace_we_are_in = cap_context.namespace_we_are_in;
    Scope* last_scope = cap_context.scope;
    Function_Implementation* last_function_being_built = cap_context.function_being_built;
    cap_context.function_being_built = implementation;
    cap_context.namespace_we_are_in = function->namespace_id;
    cap_context.scope = &implementation->body;

    for (u64 i = 0; i < parameter_count; i++) {
        Expression* parameter_expression = &parameters[i];
        Type* parameter_type = &parameter_expression->type;
        Ast* function_ast = function->ast;
        massert(function_ast->kind == ast_function_declaration, str("expected function declaration"));
        Ast* parameter_ast = &function_ast->function_declaration.parameters[i];
        String parameter_name = parameter_ast->function_declaration_parameters.name;
        Type var_type = sem_type_new_allocator_ids(parameter_type);
        Variable* var = sem_add_variable(parameter_name, var_type, parameter_ast);
        implementation->parameters[i] = var;
    }

    Type* func_return_type = &function->function_type.function.return_types[0];

    Type* return_types = cap_alloc(function->function_type.function.return_types_count * sizeof(Type));
    for (u64 i = 0; i < function->function_type.function.return_types_count; i++) {
        Type* return_type = &function->function_type.function.return_types[i];
        Type return_type_new = sem_type_new_allocator_ids(return_type);
        return_types[i] = return_type_new;
    }
    implementation->return_types = return_types;
    implementation->return_types_count = function->function_type.function.return_types_count;

    Ast* function_ast = function->ast;
    massert(function_ast->kind == ast_function_declaration, str("expected function declaration"));
    Ast* body_ast = function_ast->function_declaration.body;
    massert(body_ast->kind == ast_function_scope, str("expected function body to be a scope"));

    sem_scope_parse_statements(body_ast, &implementation->body);

    ptr_append(function->implementations, function->implementations_count, function->implementations_capacity, implementation);

    cap_context.namespace_we_are_in = last_namespace_we_are_in;
    cap_context.scope = last_scope;
    cap_context.function_being_built = last_function_being_built;

    return implementation;
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

bool sem_assign_expression(Expression* assignee, Expression* value, Ast* ast) {
    if (assignee->kind == expression_variable_declaration) {
        Type* assignee_type = &assignee->variable_declaration.type;
        Variable* variable = sem_add_variable(assignee->variable_declaration.name, *assignee_type, assignee->ast);
        assignee->variable_declaration.variable = variable;
        if (value) {
            *value = sem_implicit_cast(value, assignee_type);
            if (value->kind == expression_invalid) return false;
            variable->initial_value = value;
        }
    } else if (value) {
        if (assignee->type.kind != type_reference) {
            log_error_ast(assignee->ast, "expected assignable value");
            return false;
        }
        if (assignee->kind == expression_variable) {
            Variable* variable = assignee->variable.variable;
            variable->lost_constant_at_ast = ast;
            variable->know_compile_time_value = false;
        }
        Type assignee_type = sem_type_dereference(&assignee->type);
        *value = sem_implicit_cast(value, &assignee_type);
        if (value->kind == expression_invalid) return false;
    } else {
        // do nothing
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

    if (values_count == 0) {
        for (u64 i = 0; i < assignees_count; i++) {
            Expression* assignee = &assignees[i];
            if (!sem_assign_expression(assignee, NULL, ast)) return (Statement){0};
        }
    } else if (assignees_count == values_count) {
        for (u64 i = 0; i < assignees_count; i++) {
            Expression* assignee = &assignees[i];
            Expression* value = &values[i];
            if (!sem_assign_expression(assignee, value, ast)) return (Statement){0};
        }
    } else if (values_count == 1 && values[0].type.kind == type_multiple_value) {
        Expression* value = &values[0];
        statement.kind = statement_assignment_multiple_values;
        statement.assignment_multiple_values.multiple_values_value = values;

        Type* value_type = &value->type;
        if (value_type->multiple_value.types_count != assignees_count) {
            log_error_ast(ast, "expected %llu assignees but got %llu", value_type->multiple_value.types_count, assignees_count);
            return (Statement){0};
        }
        values = cap_alloc(value_type->multiple_value.types_count * sizeof(Expression));
        for (u64 i = 0; i < value_type->multiple_value.types_count; i++) {
            Expression* assignee = &assignees[i];
            values[i] = sem_multiple_values_access(value, i, ast);
            if (values[i].kind == expression_invalid) return (Statement){0};
            if (!sem_assign_expression(assignee, &values[i], ast)) return (Statement){0};
        }
        statement.assignment_multiple_values.values = values;
        statement.assignment_multiple_values.assignees = assignees;
        statement.assignment_multiple_values.count = assignees_count;
        return statement;
    } else {
        log_error_ast(ast, "expected %llu assignees but got %llu", assignees_count, values_count);
    }

    statement.assignment.assignees = assignees;
    statement.assignment.values = values;
    statement.assignment.assignees_count = assignees_count;
    statement.assignment.values_count = values_count;
    return statement;
}

Statement sem_statement_return(Ast* ast) {
    massert(ast->kind == ast_return, str("expected ast_return"));
    Statement statement = {0};
    statement.kind = statement_return;
    statement.ast = ast;
    Expression* values = cap_alloc(ast->return_.values_count * sizeof(Expression));
    Type* return_types = cap_context.function_being_built->return_types;
    if (ast->return_.values_count != cap_context.function_being_built->return_types_count) {
        log_error_ast(ast, "expected %llu return values but got %llu", cap_context.function_being_built->return_types_count, ast->return_.values_count);
        return (Statement){0};
    }
    for (u64 i = 0; i < ast->return_.values_count; i++) {
        Ast* value = &ast->return_.values[i];
        Expression value_expression = sem_expression_parse(value);
        if (value_expression.kind == expression_invalid) return (Statement){0};
        Type* return_type = &return_types[i];
        value_expression = sem_implicit_cast(&value_expression, return_type);
        if (value_expression.kind == expression_invalid) return (Statement){0};
        values[i] = value_expression;
    }
    statement.return_.values = values;
    statement.return_.values_count = ast->return_.values_count;
    return statement;
}

Statement sem_statement_parse(Ast* ast) {
    switch (ast->kind) {
        case ast_return:
            return sem_statement_return(ast);
        case ast_assignment:
            return sem_statement_assignment(ast);
        case ast_struct:
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
        case ast_struct_field:
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

void __sem_find_functions_with_name_and_namespace(String name, String* namespaces, u64 namespaces_count, Variable*** out_variables, u64* out_capacity,
                                                  u64* out_count, Scope* scope) {
    u64 variables_count = *out_count;
    u64 variables_capacity = *out_capacity;
    Variable** variables = *out_variables;
    for (u64 i = 0; i < scope->variables_count; i++) {
        Variable* variable = scope->variables[i];
        if (variable->type.kind != type_function) continue;
        if (string_equal(variable->name, name) && sem_variable_fits_namespace(variable, namespaces, namespaces_count)) {
            ptr_append(variables, variables_count, variables_capacity, variable);
        };
    }
    *out_count = variables_count;
    *out_capacity = variables_capacity;
    *out_variables = variables;
    if (scope->parent != NULL) {
        __sem_find_functions_with_name_and_namespace(name, namespaces, namespaces_count, out_variables, out_capacity, out_count, scope->parent);
    }
}

Variable** sem_find_functions_with_name_and_namespace(String name, String* namespaces, u64 namespaces_count, u64* out_count) {
    Scope* scope = cap_context.scope;
    u64 variables_capacity = 0;
    u64 variables_count = 0;
    Variable** variables = NULL;
    __sem_find_functions_with_name_and_namespace(name, namespaces, namespaces_count, &variables, &variables_capacity, &variables_count, scope);
    *out_count = variables_count;
    return variables;
}

void sem_complete_variable_type(Variable* variable) {
    if (variable->is_type_complete) return;
    u64 namespace_we_are_in = variable->namespace;
    Type* type = &variable->type;
    Type_Kind kind = type->kind;
    switch (kind) {
        case type_multiple_value: {
            mabort(str("type should always be complete"));
            break;
        }
        case type_function: {
            Type_Function* function = &type->function;
            massert(variable->know_compile_time_value, str("expected compile time value"));
            Function* f = *(Function**)variable->compile_time_value;
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
        case type_float:
        case type_uint:
        case type_int:
        case type_invalid:
        case type_reference:
        case type_pointer: {
            mabort(str("type should always be complete"));
            break;
        }
    }
    variable->is_type_complete = true;
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
    Type lhs = sem_type_parse(ast->biop.lhs);
    if (lhs.kind == type_invalid) return (Expression){0};
    Ast* rhs = ast->biop.rhs;
    massert(rhs->kind == ast_variable, str("expected ast_variable"));
    if (rhs->variable.namespaces_count != 0) {
        log_error_ast(ast, "can't use namespaces in variable declaration");
        return (Expression){0};
    }
    String name = rhs->variable.name;
    expr.type = sem_type_reference(&lhs, ast);
    expr.variable_declaration.name = name;
    expr.variable_declaration.type = lhs;
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
    Type expr_type = variable->type;
    expr_type = sem_type_reference(&expr_type, ast);
    if (expr_type.kind == type_invalid) return (Expression){0};
    // TODO: set allocator to stack
    expr.type = expr_type;
    expr.ast = ast;
    return expr;
}

Expression sem_get_value_if(Expression* expr) {
    if (expr->type.kind != type_reference) return *expr;
    return sem_dereference(expr);
}

Expression sem_expression_int_parse(Ast* ast) {
    massert(ast->kind == ast_int, str("expected ast_int"));
    Expression expr = {0};
    expr.kind = expression_int;
    expr.int_value.value = ast->int_value.value;
    expr.type = sem_type_int_literal();
    expr.ast = ast;
    return expr;
}

Expression sem_expression_float_parse(Ast* ast) {
    massert(ast->kind == ast_float, str("expected ast_float"));
    Expression expr = {0};
    expr.kind = expression_float;
    expr.float_value.value = ast->float_value.value;
    expr.type = sem_type_float_literal();
    expr.ast = ast;
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
    value = sem_get_value_if(&value);
    if (value.kind == expression_invalid) return (Expression){0};
    if (value.type.kind != type_type && value.type.kind != type_pointer) {
        log_error_ast(ast, "expected pointer or type");
        return (Expression){0};
    }
    Expression expr = sem_dereference(&value);
    if (expr.kind == expression_invalid) return (Expression){0};
    expr.ast = ast;
    return expr;
}

Expression sem_expression_multiply(Ast* ast) {
    massert(ast->kind == ast_multiply, str("expected ast_multiply"));
    Expression lhs = sem_expression_parse(ast->biop.lhs);
    lhs = sem_get_value_if(&lhs);
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
    if (function == NULL) return (Expression){0};
    return sem_function_call(function, parameters, parameter_count, ast);
}

Expression sem_expression_struct_parse(Ast* ast) {
    massert(false, str("not implemented"));
    return (Expression){0};
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
        case ast_struct: {
            return sem_expression_struct_parse(ast);
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
        case ast_struct_field:
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
    Expression deref = {0};
    deref.kind = expression_dereference;
    deref.ast = expr->ast;
    deref.dereference.expr = cap_alloc(sizeof(Expression));
    *deref.dereference.expr = *expr;
    if (expr->type.kind == type_type) {
        deref.type = sem_type_type();
    } else {
        deref.type = sem_type_dereference(&expr->type);
        if (deref.type.kind == type_invalid) return (Expression){0};
    }
    return deref;
}

Expression sem_reference(Expression* expr) {
    Expression ref = {0};
    ref.kind = expression_reference;
    ref.ast = expr->ast;
    ref.reference.expr = cap_alloc(sizeof(Expression));
    *ref.reference.expr = *expr;
    if (expr->type.kind != type_reference) {
        log_error_ast(expr->ast, "can't get address of non reference value");
        return (Expression){0};
    }
    if (expr->kind == expression_variable) {
        Variable* var = expr->variable.variable;
        var->lost_constant_at_ast = expr->ast;
        var->know_compile_time_value = false;
    }
    Type underlying_type = sem_type_underlying_type(&expr->type);
    ref.type = sem_type_pointer(&underlying_type, expr->ast);
    if (ref.type.kind == type_invalid) return (Expression){0};
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

Expression sem_passthrough(Expression* expr) {
    Expression passthrough = {0};
    passthrough.kind = expression_passthrough;
    passthrough.ast = expr->ast;
    passthrough.passthrough.expr = cap_alloc(sizeof(Expression));
    *passthrough.passthrough.expr = *expr;
    passthrough.type = expr->type;
    return passthrough;
}

Expression _sem_implicit_cast(Expression* expr, Type* type, bool connect_allocator) {
    if (sem_type_equal(type, &expr->type)) return *expr;
    if (sem_can_implicit_cast(expr, type)) {
        Expression cast = _sem_cast(expr, type, connect_allocator);
        if (cast.kind == expression_invalid) return (Expression){0};
        return cast;
    }
    Type expr_type = expr->type;
    if (expr_type.kind == type_reference && type->kind != type_reference) {
        expr_type = sem_type_dereference(&expr_type);
    }
    String expr_str = sem_type_to_string(&expr_type);
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
    implmentation->is_complete = true;
    implmentation->body.parent = function.scope_created_in;
    implmentation->return_types_count = 1;
    implmentation->return_types = cap_alloc(sizeof(Type) * implmentation->return_types_count);
    implmentation->return_types[0] = sem_int_type(32);
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
