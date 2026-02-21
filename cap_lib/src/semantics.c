#include "cap/semantics.h"

#include "cap.h"
#include "cap/ast.h"

String sem_type_to_string(Type* type) {
    switch (type->origin.kind) {
        case type_variable_origin: {
            Variable* variable = type->origin.variable;
            String name = variable->name;
            return name;
        }
        case type_intrisic_origin: {
            break;
        }
        case type_call_origin: {
            massert(false, str("not implemented"));
            break;
        }
    }
    switch (type->kind) {
        case type_int_literal: {
            return str("int_literal");
        }
        case type_float_literal: {
            return str("float_literal");
        }
        case type_type: {
            return str("type");
        }
        case type_struct: {
            String str = str("struct{");

            for (u64 i = 0; i < type->struct_.field_count; i++) {
                str = string_append(str, str("field("));

                Type* field_type = type->struct_.field_types[i];
                String field_type_str = sem_type_to_string(field_type);
                str = string_append(str, field_type_str);

                str = string_append(str, str(" "));

                String field_name = type->struct_.field_names[i];
                str = string_append(str, field_name);

                str = string_append(str, str(")"));
                if (i != type->struct_.field_count - 1) str = string_append(str, str(", "));
            }

            str = string_append(str, str("}"));
            return str;
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
    type.allocator_id = sem_get_new_allocator_id();
    type.origin.kind = type_intrisic_origin;
    type.origin.previous = NULL;
    return type;
}

Type sem_void_type() {
    Type type = {0};
    type.kind = type_void;
    type.allocator_id = sem_get_new_allocator_id();
    type.origin.kind = type_intrisic_origin;
    type.origin.previous = NULL;
    return type;
}

Type sem_int_type(i64 bits) {
    Type type = {0};
    type.kind = type_int;
    type.int_.bits = bits;
    type.allocator_id = sem_get_new_allocator_id();
    type.origin.kind = type_intrisic_origin;
    type.origin.previous = NULL;
    return type;
}

Type sem_uint_type(i64 bits) {
    Type type = {0};
    type.kind = type_uint;
    type.uint.bits = bits;
    type.allocator_id = sem_get_new_allocator_id();
    type.origin.kind = type_intrisic_origin;
    type.origin.previous = NULL;
    return type;
}

Type sem_float_type(i64 bits) {
    Type type = {0};
    type.kind = type_float;
    type.float_.bits = bits;
    type.allocator_id = sem_get_new_allocator_id();
    type.origin.kind = type_intrisic_origin;
    type.origin.previous = NULL;
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
    type.origin.kind = type_intrisic_origin;
    type.origin.previous = NULL;
    return type;
}

Type sem_type_struct(String* field_names, u64 field_names_count, Type** field_types, u64 field_types_count) {
    Type type = {0};
    type.kind = type_struct;
    type.struct_.field_names = field_names;
    type.struct_.field_types = field_types;
    type.struct_.field_count = field_types_count;
    type.allocator_id = sem_get_new_allocator_id();
    type.origin.kind = type_intrisic_origin;
    type.origin.previous = NULL;
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
    type.origin.kind = type_intrisic_origin;
    type.origin.previous = NULL;
    return type;
}

Type sem_type_int_literal() {
    Type type = {0};
    type.kind = type_int_literal;
    type.allocator_id = sem_get_new_allocator_id();
    type.origin.kind = type_intrisic_origin;
    type.origin.previous = NULL;
    return type;
}

Type sem_type_float_literal() {
    Type type = {0};
    type.kind = type_float_literal;
    type.allocator_id = sem_get_new_allocator_id();
    type.origin.kind = type_intrisic_origin;
    type.origin.previous = NULL;
    return type;
}

Type sem_type_invalid() {
    Type type = {0};
    type.kind = type_invalid;
    type.allocator_id = sem_get_new_allocator_id();
    type.origin.kind = type_intrisic_origin;
    type.origin.previous = NULL;
    return type;
}

Type sem_type_multiple_value(Type* types, u64 types_count) {
    Type type = {0};
    type.kind = type_multiple_value;
    type.multiple_value.types = types;
    type.multiple_value.types_count = types_count;
    type.allocator_id = sem_get_new_allocator_id();
    type.origin.previous = NULL;
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
        case type_struct: {
            Type new_type = *type;
            new_type.allocator_id = sem_get_new_allocator_id();
            return new_type;
        }
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
    return sem_type_equal(&ptr_underlying_type, underlying_type, false, true);
}

bool sem_type_is_ptr_to(Type* type, Type* underlying_type) {
    massert(type->kind == type_reference, str("expected reference"));
    Type ref_underlying_type = sem_type_underlying_type(type);
    return sem_type_equal(&ref_underlying_type, underlying_type, false, true);
}

bool __sem_origin_match(Type_Origin* origin_a, Type_Origin* origin_b, bool try_a, bool try_b) {
    if (origin_a->previous != NULL && try_a) {
        if (__sem_origin_match(origin_a->previous, origin_b, try_a, false)) return true;
    }
    if (origin_b->previous != NULL && try_b) {
        if (__sem_origin_match(origin_a, origin_b->previous, false, try_b)) return true;
    }
    if (origin_a->kind != origin_b->kind) return false;
    switch (origin_a->kind) {
        case type_variable_origin: {
            Variable* variable_a = origin_a->variable;
            Variable* variable_b = origin_b->variable;
            return variable_a == variable_b;
        }
        case type_intrisic_origin: {
            return true;
        }
        case type_call_origin: {
            Type_Call_Origin* call_origin_a = origin_a->call_origin;
            Type_Call_Origin* call_origin_b = origin_b->call_origin;
            if (call_origin_a->function != call_origin_b->function) return false;
            if (call_origin_a->parameter_count != call_origin_b->parameter_count) return false;
            for (u64 i = 0; i < call_origin_a->parameter_count; i++) {
                Type* parameter_a = &call_origin_a->parameter_types[i];
                Type* parameter_b = &call_origin_b->parameter_types[i];
                massert(sem_type_equal(parameter_a, parameter_b, false, true), str("expected equal types"));
                void* compile_time_value_a = call_origin_a->parameter_compile_time_values[i];
                void* compile_time_value_b = call_origin_b->parameter_compile_time_values[i];
                if (!sem_comile_time_value_is_equal(parameter_a, compile_time_value_a, compile_time_value_b)) return false;
            }
            return true;
        }
    }
}

bool _sem_origin_match(Type_Origin* origin_a, Type_Origin* origin_b) {
    return __sem_origin_match(origin_a, origin_b, true, true);
}

bool sem_type_equal(Type* type_a, Type* type_b, bool check_allocator, bool check_origin) {
    if (type_a->kind != type_b->kind) return false;
    if (check_origin) {
        if (!_sem_origin_match(&type_a->origin, &type_b->origin)) return false;
    }
    if (check_allocator) {
        u64 allocator_id_a = type_a->allocator_id;
        u64 allocator_id_b = type_b->allocator_id;
        Allocator* allocator_a = sem_get_allocator(allocator_id_a);
        Allocator* allocator_b = sem_get_allocator(allocator_id_b);
        if (allocator_a != allocator_b) return false;
    }
    switch (type_a->kind) {
        case type_struct: {
            Type_Struct* struct_a = &type_a->struct_;
            Type_Struct* struct_b = &type_b->struct_;
            if (struct_a->field_count != struct_b->field_count) return false;
            for (u64 i = 0; i < struct_a->field_count; i++) {
                String field_name_a = struct_a->field_names[i];
                String field_name_b = struct_b->field_names[i];
                if (!string_equal(field_name_a, field_name_b)) return false;
                Type* field_type_a = struct_a->field_types[i];
                Type* field_type_b = struct_b->field_types[i];
                if (!sem_type_equal(field_type_a, field_type_b, check_allocator, check_origin)) return false;
            }
            return true;
        }
        case type_multiple_value: {
            Type_Multiple_Value* multi_value_a = &type_a->multiple_value;
            Type_Multiple_Value* multi_value_b = &type_b->multiple_value;
            if (multi_value_a->types_count != multi_value_b->types_count) return false;
            for (u64 i = 0; i < multi_value_a->types_count; i++) {
                Type* type_a = &multi_value_a->types[i];
                Type* type_b = &multi_value_b->types[i];
                if (!sem_type_equal(type_a, type_b, check_allocator, check_origin)) return false;
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
            return sem_type_equal(underlying_type_a, underlying_type_b, check_allocator, check_origin);
        }
        case type_pointer: {
            Type* underlying_type_a = type_a->pointer.underlying_type;
            Type* underlying_type_b = type_b->pointer.underlying_type;
            return sem_type_equal(underlying_type_a, underlying_type_b, check_allocator, check_origin);
        }
        case type_int_literal:
        case type_float_literal:
        case type_type:
        case type_void:
        case type_invalid:
            return true;
    }
}

bool _sem_get_dependent_variable_in_expression(Expression* expression, Expression*** out_dependent_expressions, u64* out_count, u64* out_capacity,
                                               Function_Implementation*** out_implementations, u64* out_implementations_count,
                                               u64* out_implementations_capacity) {
    switch (expression->kind) {
        case expression_compile_time_value: {
            return true;
        }
        case expression_uint_type:
        case expression_int_type: {
            return true;
        }
        case expression_alloc: {
            Expression* count = expression->alloc.count;
            return _sem_get_dependent_variable_in_expression(count, out_dependent_expressions, out_count, out_capacity, out_implementations,
                                                             out_implementations_count, out_implementations_capacity);
        }
        case expression_struct_field_access: {
            Expression* struct_value = expression->struct_field_access.struct_value;
            return _sem_get_dependent_variable_in_expression(struct_value, out_dependent_expressions, out_count, out_capacity, out_implementations,
                                                             out_implementations_count, out_implementations_capacity);
        }
        case expression_passthrough: {
            Expression* expr = expression->passthrough.expr;
            return _sem_get_dependent_variable_in_expression(expr, out_dependent_expressions, out_count, out_capacity, out_implementations,
                                                             out_implementations_count, out_implementations_capacity);
        }
        case expression_multiple_values_access: {
            Expression* multiple_values_value = expression->multiple_values_access.multiple_values_value;
            return _sem_get_dependent_variable_in_expression(multiple_values_value, out_dependent_expressions, out_count, out_capacity, out_implementations,
                                                             out_implementations_count, out_implementations_capacity);
        }
        case expression_variable: {
            ptr_append(*out_dependent_expressions, *out_count, *out_capacity, expression);
            return true;
        }
        case expression_dereference: {
            return _sem_get_dependent_variable_in_expression(expression->dereference.expr, out_dependent_expressions, out_count, out_capacity,
                                                             out_implementations, out_implementations_count, out_implementations_capacity);
        }
        case expression_cast: {
            return _sem_get_dependent_variable_in_expression(expression->cast.expr, out_dependent_expressions, out_count, out_capacity, out_implementations,
                                                             out_implementations_count, out_implementations_capacity);
        }
        case expression_reference: {
            return _sem_get_dependent_variable_in_expression(expression->reference.expr, out_dependent_expressions, out_count, out_capacity,
                                                             out_implementations, out_implementations_count, out_implementations_capacity);
        }
        case expression_function_call_external: {
            log_error_ast(expression->ast, "external function calls are not supported at compile time");
            return false;
        }
        case expression_function_call_internal: {
            Function_Implementation* implementation = expression->internal_call.implementation;
            ptr_append(*out_implementations, *out_implementations_count, *out_implementations_capacity, implementation);
            bool ret = true;
            for (u64 i = 0; i < expression->internal_call.parameter_count; i++) {
                Expression* parameter = &expression->internal_call.parameters[i];
                bool res = _sem_get_dependent_variable_in_expression(parameter, out_dependent_expressions, out_count, out_capacity, out_implementations,
                                                                     out_implementations_count, out_implementations_capacity);
                ret = ret && res;
            }
            return ret;
        }
        case expression_struct: {
            bool ret = true;
            for (u64 i = 0; i < expression->struct_.field_count; i++) {
                Expression* field_type = &expression->struct_.field_types[i];
                bool res = _sem_get_dependent_variable_in_expression(field_type, out_dependent_expressions, out_count, out_capacity, out_implementations,
                                                                     out_implementations_count, out_implementations_capacity);
                ret = ret && res;
            }
            return ret;
        }
        case expression_variable_declaration: {
            log_error_ast(expression->ast, "variable declarations are not supported at compile time");
            return false;
        }
        case expression_int:
        case expression_float:
            return true;
        case expression_invalid:
            log_error_ast(expression->ast, "invalid expression not supported at compile time");
            return false;
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
            massert(variable->compile_time_value, str("expected compile time value"));
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
            if (sem_type_equal(underlying_type, current_type, false, false)) {
                return sem_evaluate_expression(underlying_expr);
            }
            return NULL;
        }
        case expression_compile_time_value: {
            return expression->compile_time_value.value;
        }
        default:
            return NULL;
    }
}

bool sem_comile_time_value_is_equal(Type* type, void* value_a, void* value_b) {
    if (value_a == NULL || value_b == NULL) return false;
    switch (type->kind) {
        case type_struct: {
            Type_Struct* struct_ = &type->struct_;
            // TODO:
            massert(false, str("not implemented"));
            return false;
        }
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
            memset(a, 0, bytes);
            char b[bytes];
            memset(b, 0, bytes);
            memcpy(a, value_a, bytes);
            memcpy(b, value_b, bytes);
            return memcmp(a, b, bytes) == 0;
        }
        case type_uint: {
            i64 bits = type->uint.bits;
            i64 bytes = bits / 8;
            char a[bytes];
            memset(a, 0, bytes);
            char b[bytes];
            memset(b, 0, bytes);
            memcpy(a, value_a, bytes);
            memcpy(b, value_b, bytes);
            return memcmp(a, b, bytes) == 0;
        }
        case type_float: {
            i64 bits = type->float_.bits;
            i64 bytes = bits / 8;
            char a[bytes];
            memset(a, 0, bytes);
            char b[bytes];
            memset(b, 0, bytes);
            memcpy(a, value_a, bytes);
            memcpy(b, value_b, bytes);
            return memcmp(a, b, bytes) == 0;
        }
        case type_type: {
            Type** a = (Type**)value_a;
            Type** b = (Type**)value_b;
            Type* a_type = *a;
            Type* b_type = *b;
            return sem_type_equal(a_type, b_type, false, true);
        }
        case type_reference: {
            Type* underlying_type = type->reference.underlying_type;
            void* a = *(void**)value_a;
            void* b = *(void**)value_b;
            return sem_comile_time_value_is_equal(underlying_type, a, b);
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
    bool res = _sem_get_dependent_variable_in_expression(expression, &variable_exprs, &variable_exprs_count, &variable_exprs_capacity, &implementations,
                                                         &implementations_count, &implementations_capacity);
    if (!res) return NULL;
    for (u64 i = 0; i < implementations_count; i++) {
        Function_Implementation* implementation = implementations[i];
        sem_complete_implementation(implementation);
        if (!implementation->is_complete) {
            log_error_ast(expression->ast, "could not evaluate expression because function implementation is not complete");
            return NULL;
        }
        if (implementation->lost_compile_time_evaluatable_at) {
            log_error_ast(expression->ast, "could not evaluate expression because function implementation is not evaluateable at compile time");
            log_info_ast(implementation->lost_compile_time_evaluatable_at, "expression making function not compile time evaluable");
            return NULL;
        }
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
        if (variable->compile_time_value) {
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
        sem_set_variable_compile_time_value(variable, value);
    }
    void* value = _sem_evaluate_expression_trivially(expression);
    if (value != NULL) return value;
    return llvm_evaluate_expression(expression);
}

void sem_set_variable_compile_time_value(Variable* var, void* value) {
    massert(!var->compile_time_value, str("variable already has a compile time value"));
    if (var->type.kind == type_type) {
        Type** type_ptr = value;
        Type* type = *type_ptr;
        massert(type != NULL, str("type was found to be null"));
        Type type_new = *type;

        Type_Origin previous = type_new.origin;
        Type_Origin new_origin = {0};
        new_origin.kind = type_variable_origin;
        new_origin.variable = var;
        new_origin.previous = cap_alloc(sizeof(Type_Origin));
        *new_origin.previous = previous;
        type_new.origin = new_origin;

        Type* new_type_ptr = cap_alloc(sizeof(Type));
        *new_type_ptr = type_new;
        *(Type**)value = new_type_ptr;
    }
    var->compile_time_value = value;
}

void* sem_evaluate_expression(Expression* expression) {
    if (expression->value != NULL) return expression->value;
    void* value = _sem_evaluate_expression(expression);
    if (value == NULL) return NULL;
    if (expression->type.kind == type_type) {
        Type** type_ptr = value;
        Type* type = *type_ptr;
        massert(type != NULL, str("type was found to be null"));
        Type type_new = sem_type_new_allocator_ids(type);
        Type* new_type = cap_alloc(sizeof(Type));
        *new_type = type_new;
        Type** new_type_ptr = cap_alloc(sizeof(Type*));
        *new_type_ptr = new_type;
        value = new_type_ptr;
    }
    expression->value = value;
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

    String name = function_declaration->name;

    for (u64 i = 0; i < function_declaration->parameters_count; i++) {
        Ast* parameter_ast = &function_declaration->parameters[i];
        String* name = &parameter_ast->function_declaration_parameters.name;
        for (u64 j = i + 1; j < function_declaration->parameters_count; j++) {
            Ast* parameter_ast2 = &function_declaration->parameters[j];
            String* name2 = &parameter_ast2->function_declaration_parameters.name;
            if (string_equal(*name, *name2)) {
                log_error_ast(parameter_ast2, "parameter %.*s already declared", str_info(*name));
                return (Function){0};
            }
        }
    }

    Ast* body_ast = ast->function_declaration.body;
    if (body_ast != NULL) {
        return sem_create_function_internal(name, ast);
    } else {
        return sem_create_function_external(name, ast);
    }
}

bool sem_type_is_valid_external_type(Type* type) {
    switch (type->kind) {
        case type_int_literal:
        case type_float_literal:
        case type_invalid:
        case type_multiple_value:
        case type_type:
            return false;
        case type_pointer:
            return sem_type_is_valid_external_type(type->pointer.underlying_type);
        case type_reference:
            return sem_type_is_valid_external_type(type->reference.underlying_type);
        case type_struct: {
            for (u64 i = 0; i < type->struct_.field_count; i++) {
                Type* field_type = type->struct_.field_types[i];
                if (!sem_type_is_valid_external_type(field_type)) return false;
            }
            return true;
        }
        case type_int:
        case type_float:
        case type_uint:
        case type_void:
            return true;
    }
}

Function sem_create_function_external(String name, Ast* ast) {
    Function function = {0};
    function.name = name;
    function.scope_created_in = cap_context.scope;
    function.ast = ast;
    function.kind = function_external;
    function.namespace_id = cap_context.namespace_we_are_in;

    massert(ast->kind == ast_function_declaration, str("expected ast_function_declaration"));
    u64 return_count = ast->function_declaration.return_types_count;
    if (return_count != 1) {
        log_error_ast(ast, "external functions should have only 1 return type, but got %llu", return_count);
        return (Function){0};
    }
    Type return_type = sem_type_parse(&ast->function_declaration.return_types[0]);
    if (return_type.kind == type_invalid) return (Function){0};
    if (!sem_type_is_valid_external_type(&return_type)) {
        log_error_ast(ast, "external function return type is not a external valid return type");
        return (Function){0};
    }
    function.external.return_type = return_type;

    u64 parameter_count = ast->function_declaration.parameters_count;
    Type* parameter_types = cap_alloc(parameter_count * sizeof(Type));
    String* parameter_names = cap_alloc(parameter_count * sizeof(String));
    for (u64 i = 0; i < parameter_count; i++) {
        Ast* parameter_ast = &ast->function_declaration.parameters[i];
        Type parameter_type = sem_type_parse(parameter_ast);
        if (parameter_type.kind == type_invalid) return (Function){0};
        if (!sem_type_is_valid_external_type(&parameter_type)) {
            log_error_ast(ast, "external function parameter type is not a external valid parameter type");
            return (Function){0};
        }
        parameter_types[i] = parameter_type;
        parameter_names[i] = parameter_ast->function_declaration_parameters.name;
    }
    function.external.parameter_types = parameter_types;
    function.external.parameter_names = parameter_names;
    function.external.parameter_count = parameter_count;

    return function;
}

Function sem_create_function_internal(String name, Ast* ast) {
    Function function = {0};
    function.name = name;
    function.scope_created_in = cap_context.scope;
    function.kind = function_internal;

    function.internal.implementations_capacity = 1;
    function.internal.implementations_count = 0;
    function.internal.implementations = cap_alloc(sizeof(Function_Implementation));
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

void sem_connect_allocator_ids(Allocator_Id id1, Allocator_Id id2, Ast* ast_for_error) {
    Allocator* data1 = cap_context.allocator_map.allocator[id1];
    Allocator* data2 = cap_context.allocator_map.allocator[id2];
    bool replace_data1 = false;
    if (data1->variable == NULL && data2->variable == NULL) {
        replace_data1 = true;
    } else if (data1->variable != NULL && data2->variable == NULL) {
        replace_data1 = false;
    } else if (data1->variable == NULL && data2->variable != NULL) {
        replace_data1 = true;
    } else {
        log_error_ast(ast_for_error, "can't connect up allocators both are defined already: %.*s and %.*s", str_info(data1->variable->name),
                      str_info(data2->variable->name));
        return;
    }

    for (u64 i = 0; i < cap_context.allocator_map.allocator_count; i++) {
        Allocator** data = &cap_context.allocator_map.allocator[i];
        if (replace_data1) {
            if (*data == data1) *data = data2;
        } else {
            if (*data == data2) *data = data1;
        }
    }
}

void sem_set_id_allocator(Allocator_Id id, Allocator* allocator) {
    Allocator* old_allocator = cap_context.allocator_map.allocator[id];
    massert(old_allocator->variable == NULL, str("expected no allocator to be set"));
    *(cap_context.allocator_map.allocator[id]) = *allocator;
}

Allocator* sem_get_allocator(Allocator_Id id) {
    return cap_context.allocator_map.allocator[id];
}

bool _sem_namespace_fits_namespace(u64 namespace, String* namespaces, u64 namespaces_count, u64 namespace_we_are_in) {
    Cap_Folder* folder = cap_context.folders[namespace_we_are_in];
    if (namespace == namespace_we_are_in && namespaces_count == 0) return true;
    for (u64 j = 0; j < folder->folders_count; j++) {
        Cap_Folder* child_folder = folder->folders[j];
        String alias = folder->folder_namespace_aliases[j];
        if (string_equal(alias, str(""))) {
            if (_sem_namespace_fits_namespace(namespace, namespaces, namespaces_count, child_folder->namespace_id)) return true;
        }
    }
    if (namespaces_count == 0) return false;
    String namespace_alias = namespaces[0];
    for (u64 j = 0; j < folder->folders_count; j++) {
        Cap_Folder* child_folder = folder->folders[j];
        String alias = folder->folder_namespace_aliases[j];
        if (string_equal(alias, namespace_alias)) {
            if (_sem_namespace_fits_namespace(namespace, namespaces + 1, namespaces_count - 1, child_folder->namespace_id)) return true;
        }
    }
    return false;
}

bool sem_namespace_fits_namespace(u64 namespace, String* namespaces, u64 namespaces_count) {
    u64 namespace_we_are_in = cap_context.namespace_we_are_in;
    return _sem_namespace_fits_namespace(namespace, namespaces, namespaces_count, namespace_we_are_in);
}

bool sem_variable_fits_namespace(Variable* variable, String* namespaces, u64 namespaces_count) {
    u64 variable_namespace = variable->namespace;
    return sem_namespace_fits_namespace(variable_namespace, namespaces, namespaces_count);
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
    return variable;
}

Function* _sem_find_alloc_variable(Type* allocator_type, Scope* scope, Ast* ast_for_error) {
    massert(false, str("not implemented"));
    for (u64 i = 0; i < scope->alloc_function_variables_count; i++) {
        Function* alloc_function = scope->alloc_function_variables[i];
        // massert(alloc_function->parameter_count == 3, str("expected 3 parameters"));
        // Type* func_allocator_type = &function_type->function.parameter_types[0];
        // if (sem_type_equal(allocator_type, func_allocator_type, false, true)) {
        // return variable;
        // }
    }
    if (scope->parent != NULL) {
        return _sem_find_alloc_variable(allocator_type, scope->parent, ast_for_error);
    }
    if (ast_for_error) log_error_ast(ast_for_error, "could not find alloc function");
    return NULL;
}

Function* sem_find_alloc_variable(Type* allocator_type, Ast* ast_for_error) {
    Scope* scope = cap_context.scope;
    return _sem_find_alloc_variable(allocator_type, scope, ast_for_error);
}

Variable* sem_add_alloc_variable(String name, Type type, Ast* ast, Function* function) {
    massert(false, str("not implemented"));
    return NULL;
    // Variable variable = {0};
    // variable.namespace = cap_context.namespace_we_are_in;
    // variable.name = name;
    // variable.type = type;
    // variable.ast = ast;
    // variable.lost_constant_at_ast = NULL;
    //
    // massert(string_equal(name, str("alloc")), str("expected alloc"));
    //
    // u64 parameter_count = function->parameter_count;
    // if (parameter_count != 3) {
    //     log_error_ast(ast, "alloc function should have 3 parameters: ((Allocator_Type)* allocator, u64 size, u64 alignment)");
    //     return NULL;
    // }
    // massert(false, str("not implemented"));
    // return NULL;
    // Type* allocator_type = &function_type->function.parameter_types[0];
    // if (allocator_type->kind != type_pointer) {
    //     log_error_ast(ast, "expected allocator type to be a pointer");
    //     return NULL;
    // }
    //
    // Type u64_type = sem_uint_type(64);
    // Type* size_type = &function_type->function.parameter_types[1];
    // if (sem_type_equal(&u64_type, size_type, false, false)) {
    //     log_error_ast(ast, "expected u64 size");
    //     return NULL;
    // }
    //
    // Type* alignment_type = &function_type->function.parameter_types[2];
    // if (sem_type_equal(&u64_type, alignment_type, false, false)) {
    //     log_error_ast(ast, "expected u64 alignment");
    //     return NULL;
    // }
    //
    // Type* return_types = function_type->function.return_types;
    // u64 return_types_count = function_type->function.return_types_count;
    // if (return_types_count != 1) {
    //     log_error_ast(ast, "alloc function should have 1 return type: void*");
    //     return NULL;
    // }
    //
    // Type void_type = sem_void_type();
    // Type pointer_type = sem_type_pointer(&void_type, ast);
    //
    // Type* return_type = &return_types[0];
    // if (!sem_type_equal(return_type, &pointer_type, false, false)) {
    //     log_error_ast(ast, "alloc function should have return type: void*");
    //     return NULL;
    // }
    //
    // Function** function_ptr_ptr = cap_alloc(sizeof(Function*));
    // *function_ptr_ptr = function;
    //
    // Variable* variable_ptr = cap_alloc(sizeof(Variable));
    // *variable_ptr = variable;
    // sem_set_variable_compile_time_value(variable_ptr, function_ptr_ptr);
    // Scope* scope = cap_context.scope;
    // ptr_append(scope->alloc_function_variables, scope->alloc_function_variables_count, scope->alloc_function_variables_capacity, variable_ptr);
    // return variable_ptr;
}

Function* sem_add_function(Function* function) {
    Function* function_ptr = cap_alloc(sizeof(Function));
    *function_ptr = *function;
    Scope* scope = cap_context.scope;
    ptr_append(scope->function_variables, scope->function_variables_count, scope->function_variables_capacity, function_ptr);
    return function_ptr;
}

Variable* sem_add_variable(String name, Type type, Ast* ast) {
    Variable variable = {0};
    variable.namespace = cap_context.namespace_we_are_in;
    variable.name = name;
    variable.type = type;
    variable.ast = ast;
    variable.lost_constant_at_ast = NULL;

    Scope* scope = cap_context.scope;
    Variable* existing = NULL;
    for (u64 i = 0; i < scope->variables_count; i++) {
        Variable* v = scope->variables[i];
        if (string_equal(v->name, variable.name) && sem_variable_fits_namespace(v, NULL, 0)) {
            existing = v;
            break;
        }
    }

    if (existing != NULL) {
        log_error_ast(variable.ast, "variable %.*s already declared", str_info(variable.name));
        return NULL;
    }

    Variable* variable_ptr = cap_alloc(sizeof(Variable));
    *variable_ptr = variable;
    ptr_append(scope->variables, scope->variables_count, scope->variables_capacity, variable_ptr);
    return variable_ptr;
}

void _sem_value_propagate(void* value, Type* type, Expression* expr, bool* is_match, Expression* casted_parameters_exprs, Variable** parameter_variables,
                          u64 func_parameters_count) {
    switch (expr->kind) {
        case expression_passthrough: {
            Expression* passthrough_expr = expr->passthrough.expr;
            _sem_value_propagate(value, type, passthrough_expr, is_match, casted_parameters_exprs, parameter_variables, func_parameters_count);
            break;
        }
        case expression_dereference: {
            Expression* derefed_expr = expr->dereference.expr;
            void** value_ptr = cap_alloc(sizeof(void*));
            *value_ptr = value;
            Type reference_type = sem_type_reference(type, expr->ast);
            _sem_value_propagate(value_ptr, &reference_type, derefed_expr, is_match, casted_parameters_exprs, parameter_variables, func_parameters_count);
            break;
        }
        case expression_function_call_internal: {
            for (u64 i = 0; i < expr->internal_call.parameter_count; i++) {
                Expression* parameter = &expr->internal_call.parameters[i];
                void* value_p = sem_evaluate_expression(parameter);
                if (value == NULL) return;
                // _sem_value_propagate(value_p, &parameter->type, parameter, is_match, casted_parameters_exprs, parameter_variables, func_parameters_count);
            }
            break;
        }
        case expression_variable: {
            Variable* variable = expr->variable.variable;

            void* value_deref = *(void**)value;
            massert(expr->type.kind == type_reference, str("expected reference"));
            massert(type->kind == type_reference, str("expected reference"));
            Type* underlying_type = type->reference.underlying_type;
            if (!sem_type_equal(underlying_type, &variable->type, false, true)) {
                break;
            }

            // check that variable is in parse scope
            bool found = false;
            u64 var_index = 0;
            for (u64 i = 0; i < func_parameters_count; i++) {
                Variable* parameter_variable = parameter_variables[i];
                if (parameter_variable == NULL) continue;
                if (variable == parameter_variable) {
                    var_index = i;
                    found = true;
                    break;
                }
            }
            if (!found) break;

            if (variable->compile_time_value || variable->initial_value) {
                if (!variable->compile_time_value) {
                    variable->compile_time_value = sem_evaluate_expression(variable->initial_value);
                    if (variable->compile_time_value == NULL) {
                        *is_match = false;
                        break;
                    }
                }
                if (!sem_comile_time_value_is_equal(&variable->type, variable->compile_time_value, value_deref)) {
                    break;
                }
            }
            if (variable->type.kind == type_type) {
                Type** call_type_ptr = value_deref;
                Type* call_type = *call_type_ptr;
                if (call_type->kind == type_int_literal || call_type->kind == type_float_literal || call_type->kind == type_multiple_value ||
                    call_type->kind == type_reference) {
                    break;
                }
            }

            variable->compile_time_value = value_deref;
            Expression compile_time_value_expr = sem_compile_time_value_expression(value_deref, variable->type, expr->ast);
            casted_parameters_exprs[var_index] = compile_time_value_expr;
            break;
        }
        default: {
            break;
        }
    }
}

Function* sem_find_function(String name, String* namespaces, u64 namespaces_count, Expression** parameters_exprs_in_out, u64* parameter_count_in_out,
                            Type** return_types_out, u64* return_types_count_out, Ast* ast) {
    Expression* parameters_exprs = *parameters_exprs_in_out;
    u64 parameter_count = *parameter_count_in_out;
    for (u64 i = 0; i < parameter_count; i++) {
        Expression* parameter_expr = &parameters_exprs[i];
        *parameter_expr = sem_get_value_if(parameter_expr);
    }

    u64 functions_count = 0;
    Function** functions = sem_find_functions_with_name_and_namespace(name, namespaces, namespaces_count, &functions_count);
    if (functions_count == 0) {
        log_error_ast(ast, "could not find any function %.*s", str_info(name));
        return NULL;
    }
    u64 function_matches_count = 0;
    bool* function_matches = cap_alloc(functions_count * sizeof(bool));
    Expression** casted_parameters_exprs_list = cap_alloc(functions_count * sizeof(Expression*));
    u64* casted_parameters_exprs_counts = cap_alloc(functions_count * sizeof(u64));
    Type** return_type_lists = cap_alloc(functions_count * sizeof(Type*));
    u64* return_types_counts = cap_alloc(functions_count * sizeof(u64));

    for (u64 i = 0; i < functions_count; i++) {
        Function* function = functions[i];
        bool is_match = true;

        Ast* function_ast = function->ast;
        massert(function_ast->kind == ast_function_declaration, str("expected function declaration"));
        bool last_log = cap_context.log;
        cap_context.log = false;

        Scope parse_scope = {0};
        parse_scope.parent = function->scope_created_in;
        Scope* last_scope = cap_context.scope;
        cap_context.scope = &parse_scope;

        u64 func_parameters_count = function_ast->function_declaration.parameters_count;
        Expression* casted_parameters_exprs = cap_alloc(func_parameters_count * sizeof(Expression));
        Variable** parameter_variables = cap_alloc(func_parameters_count * sizeof(Variable*));
        u64 return_types_count = function_ast->function_declaration.return_types_count;

        Type* return_types = cap_alloc(return_types_count * sizeof(Type));

        bool* parameter_parsed = cap_alloc(func_parameters_count * sizeof(bool));

        Type type_type_type = sem_type_type();

        while (true) {
            bool changed = false;
            for (u64 i = 0; i < func_parameters_count; i++) {
                if (parameter_parsed[i]) continue;
                Ast* parameter_ast = &function_ast->function_declaration.parameters[i];
                Ast* type_ast = parameter_ast->function_declaration_parameters.type;
                String* name = &parameter_ast->function_declaration_parameters.name;
                Expression* parameter_expr = &parameters_exprs[i];

                Expression parameter_type_expr = sem_expression_parse(type_ast);
                parameter_type_expr = sem_get_value_if(&parameter_type_expr);
                if (parameter_type_expr.kind == expression_invalid) continue;
                if (parameter_type_expr.type.kind != type_type) {
                    is_match = false;
                    break;
                }

                Type** value_for_propagate = cap_alloc(sizeof(Type*));
                *value_for_propagate = &parameter_expr->type;
                _sem_value_propagate(value_for_propagate, &type_type_type, &parameter_type_expr, &is_match, casted_parameters_exprs, parameter_variables,
                                     func_parameters_count);
                if (!is_match) break;

                Type** parameter_type_ptr = sem_evaluate_expression(&parameter_type_expr);
                if (parameter_type_ptr == NULL) {
                    is_match = false;
                    break;
                }
                Type* parameter_type_func = *parameter_type_ptr;

                Variable* var = sem_add_variable(*name, *parameter_type_func, parameter_ast);
                massert(var != NULL, str("expected variable"));
                changed = true;
                parameter_parsed[i] = true;
                parameter_variables[i] = var;

                if (i >= parameter_count) continue;
                Expression parameter_casted = sem_implicit_cast_without_allocator(parameter_expr, parameter_type_func);
                if (parameter_casted.kind == expression_invalid) {
                    is_match = false;
                    break;
                }
                casted_parameters_exprs[i] = parameter_casted;
                var->initial_value = &casted_parameters_exprs[i];
            }
            if (!changed) break;
            if (!is_match) break;
        }
        for (u64 i = 0; i < func_parameters_count; i++) {
            Expression* parameter_expr = &casted_parameters_exprs[i];
            if (parameter_expr->kind == expression_invalid) is_match = false;
            if (!parameter_parsed[i]) is_match = false;
            if (!is_match) break;
        }

        for (u64 i = 0; i < return_types_count; i++) {
            Ast* return_type_ast = &function_ast->function_declaration.return_types[i];
            Type return_type = sem_type_parse(return_type_ast);
            if (return_type.kind == type_invalid) {
                is_match = false;
                break;
            }
            return_types[i] = return_type;
        }

        cap_context.log = last_log;
        cap_context.scope = last_scope;

        if (!is_match) continue;
        function_matches[i] = true;
        casted_parameters_exprs_list[i] = casted_parameters_exprs;
        casted_parameters_exprs_counts[i] = func_parameters_count;
        return_type_lists[i] = return_types;
        return_types_counts[i] = return_types_count;
        function_matches_count += 1;
    }
    if (function_matches_count == 0) {
        log_error_ast(ast, "could not find any function with the given parameter types");
        return NULL;
    }
    if (function_matches_count > 1) {
        log_error_ast(ast, "to many functions match with the given parameter types");
        for (u64 i = 0; i < functions_count; i++) {
            if (!function_matches[i]) continue;
            Function* function = functions[i];
            log_info_ast(function->ast, "could of meant");
        }
        return NULL;
    }

    u64 match_index = 0;
    for (u64 i = 0; i < functions_count; i++) {
        if (!function_matches[i]) continue;
        match_index = i;
    }

    Function* function = functions[match_index];
    Expression* casted_parameters_exprs = casted_parameters_exprs_list[match_index];
    u64 casted_parameters_exprs_count = casted_parameters_exprs_counts[match_index];
    Type* return_types = return_type_lists[match_index];
    u64 return_types_count = return_types_counts[match_index];
    *parameters_exprs_in_out = casted_parameters_exprs;
    *parameter_count_in_out = casted_parameters_exprs_count;
    *return_types_out = return_types;
    *return_types_count_out = return_types_count;
    return function;
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

Expression sem_compile_time_value_expression(void* value, Type type, Ast* ast) {
    Expression expr = {0};
    expr.kind = expression_compile_time_value;
    expr.value = value;
    expr.compile_time_value.value = value;
    expr.type = type;
    expr.ast = ast;
    return expr;
}

Expression sem_function_call(Function* function, Expression* parameters, Type* return_types, u64 return_types_count, u64 parameter_count, Ast* ast) {
    switch (function->kind) {
        case function_internal: {
            return sem_function_call_internal(function, parameters, return_types, return_types_count, parameter_count, ast);
        }
        case function_external: {
            return sem_function_call_external(function, parameters, return_types, return_types_count, parameter_count, ast);
        }
        case function_intrinsic: {
            return sem_function_call_intrinsic(function, parameters, return_types, return_types_count, parameter_count, ast);
        }
        case function_invalid: {
            massert(false, str("expected function to be valid"));
            return (Expression){0};
        }
    }
}

Expression sem_function_call_external(Function* function, Expression* parameters, Type* return_types, u64 return_types_count, u64 parameter_count, Ast* ast) {
    massert(return_types_count == 1, str("expected 1 return type"));
    cap_context.function_being_built->lost_compile_time_evaluatable_at = ast;

    Expression expression = {0};
    expression.kind = expression_function_call_external;
    expression.external_call.function = function;
    expression.external_call.parameters = parameters;
    expression.external_call.parameter_count = parameter_count;
    expression.ast = ast;
    Type return_type = return_types[0];
    return_type = sem_type_new_allocator_ids(&return_type);
    expression.type = return_type;
    return expression;
}

Expression sem_function_call_intrinsic(Function* function, Expression* parameters, Type* return_types, u64 return_types_count, u64 parameter_count, Ast* ast) {
    massert(function->kind == function_intrinsic, str("expected function to be intrinsic"));
    massert(false, str("not implemented"));
    return (Expression){0};
}

Expression sem_function_call_internal(Function* function, Expression* parameters, Type* return_types, u64 return_types_count, u64 parameter_count, Ast* ast) {
    massert(function->kind == function_internal, str("expected function to be internal"));

    Function_Implementation* implementation = cap_alloc(sizeof(Function_Implementation));
    implementation->function = function;
    implementation->parameter_count = parameter_count;
    implementation->parameters = cap_alloc(parameter_count * sizeof(Variable*));
    implementation->body.parent = function->scope_created_in;
    implementation->is_complete = false;
    implementation->lost_compile_time_evaluatable_at = false;

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

    for (u64 i = 0; i < return_types_count; i++) {
        Type* return_type = &return_types[i];
        Type return_type_new = sem_type_new_allocator_ids(return_type);
        return_types[i] = return_type_new;
    }
    implementation->return_types = return_types;
    implementation->return_types_count = return_types_count;

    Ast* function_ast = function->ast;
    massert(function_ast->kind == ast_function_declaration, str("expected function declaration"));
    Ast* body_ast = function_ast->function_declaration.body;
    massert(body_ast->kind == ast_function_scope, str("expected function body to be a scope"));

    ptr_append(function->internal.implementations, function->internal.implementations_count, function->internal.implementations_capacity, implementation);

    cap_context.namespace_we_are_in = last_namespace_we_are_in;
    cap_context.scope = last_scope;
    cap_context.function_being_built = last_function_being_built;

    Expression function_call_expr = {0};
    function_call_expr.kind = expression_function_call_internal;

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
    function_call_expr.internal_call.parameters = parameters;
    function_call_expr.internal_call.parameter_count = parameter_count;
    function_call_expr.internal_call.implementation = implementation;

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
        case expression_function_call_internal: {
            Function_Implementation* call_implementation = expression->internal_call.implementation;
            if (!call_implementation->is_complete) {
                // try to see if a already made implementation works for this so we don't have to make a new one
                Function* function = call_implementation->function;
                massert(function->kind == function_internal, str("expected function to be internal"));
                for (u64 i = 0; i < function->internal.implementations_count; i++) {
                    Function_Implementation* implementation = function->internal.implementations[i];
                    if (implementation->is_complete) {
                        bool is_match = true;
                        for (u64 j = 0; j < implementation->parameter_count; j++) {
                            Variable* call_parameter = call_implementation->parameters[j];
                            Variable* parameter = implementation->parameters[j];
                            Type* parameter_type = &parameter->type;
                            Type* call_parameter_type = &call_parameter->type;
                            if (!sem_type_equal(parameter_type, call_parameter_type, false, true)) {
                                is_match = false;
                                break;
                            }
                            if (parameter->compile_time_value) {
                                void* call_parameter_value = sem_evaluate_expression(call_parameter->initial_value);
                                void* parameter_value = parameter->compile_time_value;
                                if (!sem_comile_time_value_is_equal(parameter_type, call_parameter_value, parameter_value)) {
                                    is_match = false;
                                    break;
                                }
                            }
                        }
                        if (is_match) {
                            expression->internal_call.implementation = implementation;
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
    u64 error_count_start = cap_context.error_count;

    cap_context.implementation_to_complete_recursion_counter++;
    u64 max_recursion_depth = 128;
    if (cap_context.implementation_to_complete_recursion_counter > max_recursion_depth) {
        Function* function = implementation->function;
        Ast* ast = function->ast;
        log_error_ast(ast, "can't compile function hit max compile time function implementation recursion depth %llu", max_recursion_depth);
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
    implementation->is_complete = cap_context.error_count == error_count_start;
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
        if (variable == NULL) return false;
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
            variable->compile_time_value = NULL;
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

Statement sem_statement_function_declaration(Ast* ast) {
    massert(ast->kind == ast_function_declaration, str("expected ast_function_declaration"));
    Statement statement = {0};
    statement.kind = statement_function_declaration;
    statement.ast = ast;
    Function function = sem_function_parse(ast);
    if (function.kind == function_invalid) return (Statement){0};

    String name = ast->function_declaration.name;
    Function* ptr;
    if (string_equal(name, str("alloc"))) {
        // var = sem_add_alloc_variable(name, function.function_type, ast, function_ptr);
        massert(false, str("not implemented"));
        return (Statement){0};
    } else {
        ptr = sem_add_function(&function);
    }
    if (ptr == NULL) return (Statement){0};

    statement.function_declaration.function = ptr;
    return statement;
}

Statement sem_statement_parse(Ast* ast) {
    switch (ast->kind) {
        case ast_return:
            return sem_statement_return(ast);
        case ast_assignment:
            return sem_statement_assignment(ast);
        case ast_function_declaration:
            return sem_statement_function_declaration(ast);
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
        case ast_field_access:
        case ast_struct_field:
        case ast_function_call:
        case ast_function_declaration_parameter:
        case ast_top_level:
        case ast_include:
        case ast_invalid: {
            log_error_ast(ast, "unexpected ast kind in statement parse");
            return (Statement){0};
        }
    }
}

void __sem_find_functions_with_name_and_namespace(String name, String* namespaces, u64 namespaces_count, Function*** out_variables, u64* out_capacity,
                                                  u64* out_count, Scope* scope) {
    u64 variables_count = *out_count;
    u64 variables_capacity = *out_capacity;
    Function** variables = *out_variables;
    for (u64 i = 0; i < scope->function_variables_count; i++) {
        Function* variable = scope->function_variables[i];
        u64 function_namespace = variable->namespace_id;
        if (string_equal(variable->name, name) && sem_namespace_fits_namespace(function_namespace, namespaces, namespaces_count)) {
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

Function** sem_find_functions_with_name_and_namespace(String name, String* namespaces, u64 namespaces_count, u64* out_count) {
    Scope* scope = cap_context.scope;
    u64 variables_capacity = 0;
    u64 variables_count = 0;
    Function** variables = NULL;
    __sem_find_functions_with_name_and_namespace(name, namespaces, namespaces_count, &variables, &variables_capacity, &variables_count, scope);
    *out_count = variables_count;
    return variables;
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
    Allocator_Id id = expr_type.allocator_id;
    sem_set_id_allocator(id, &cap_context.stack_allocator);

    expr.type = expr_type;
    expr.ast = ast;
    return expr;
}

Expression sem_get_value_if(Expression* expr) {
    if (expr->type.kind != type_reference) return *expr;
    return sem_dereference(expr);
}

Expression sem_int_expression(i64 value, Ast* ast) {
    Expression expr = {0};
    expr.kind = expression_int;
    expr.int_value.value = value;
    expr.type = sem_type_int_literal();
    expr.ast = ast;
    return expr;
}

Expression sem_expression_int_parse(Ast* ast) {
    massert(ast->kind == ast_int, str("expected ast_int"));
    return sem_int_expression(ast->int_value.value, ast);
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

Expression sem_expression_alloc(Ast* ast) {
    massert(ast->kind == ast_function_call, str("expected ast_function_call"));

    Expression alloc = {0};
    alloc.kind = expression_alloc;
    alloc.ast = ast;

    Ast* function_name_ast = ast->function_call.function_variable;
    massert(function_name_ast->kind == ast_variable, str("expected ast_variable"));
    String function_name = function_name_ast->variable.name;
    String* namespaces = function_name_ast->variable.namespaces;
    u64 namespaces_count = function_name_ast->variable.namespaces_count;

    if (!string_equal(function_name, str("alloc"))) {
        log_error_ast(function_name_ast, "expected alloc");
        return (Expression){0};
    }

    if (namespaces_count != 0) {
        log_error_ast(function_name_ast, "expected no namespaces");
        return (Expression){0};
    }

    Ast* parameters_ast = ast->function_call.parameters;
    u64 parameter_count = ast->function_call.parameters_count;

    Expression* parameters = cap_alloc(parameter_count * sizeof(Expression));
    for (u64 i = 0; i < parameter_count; i++) {
        Ast* parameter_ast = &parameters_ast[i];
        Expression parameter = sem_expression_parse(parameter_ast);
        if (parameter.kind == expression_invalid) return (Expression){0};
        parameters[i] = parameter;
    }

    if (parameter_count > 2) {
        log_error_ast(ast, "alloc should follow this call type: alloc(type type_allocate, u64 count)");
        return (Expression){0};
    }

    Expression parameter_1 = parameters[0];
    if (parameter_1.type.kind == type_type) {
        Type** parameter_type_ptr = sem_evaluate_expression(&parameter_1);
        if (parameter_type_ptr == NULL) return (Expression){0};
        Type* parameter_type = *parameter_type_ptr;
        alloc.alloc.type_to_allocate = *parameter_type;
        Type type_ptr = sem_type_pointer(parameter_type, ast);
        alloc.type = type_ptr;
        if (parameter_count == 2) {
            Expression* parameter_2 = &parameters[1];
            Type u64_type = sem_uint_type(64);
            Expression cast_p2 = sem_implicit_cast(parameter_2, &u64_type);
            if (cast_p2.kind == expression_invalid) return (Expression){0};
            alloc.alloc.count = cap_alloc(sizeof(Expression));
            *alloc.alloc.count = cast_p2;
        } else {
            Expression int_expr = sem_int_expression(1, ast);
            if (int_expr.kind == expression_invalid) return (Expression){0};
            Type u64_type = sem_uint_type(64);
            int_expr = sem_implicit_cast(&int_expr, &u64_type);
            if (int_expr.kind == expression_invalid) return (Expression){0};
            alloc.alloc.count = cap_alloc(sizeof(Expression));
            *alloc.alloc.count = int_expr;
        }
    } else {
        Ast* parameter_ast = &parameters_ast[0];
        log_error_ast(ast, "expected first parameter to be a type");
        return (Expression){0};
    }
    return alloc;
}

Expression _sem_expression_int_uint_type(Ast* ast, bool is_uint) {
    massert(ast->kind == ast_function_call, str("expected ast_function_call"));
    Expression expr = {0};
    expr.kind = is_uint ? expression_uint_type : expression_int_type;
    expr.type = sem_type_type();
    expr.ast = ast;

    Ast* function_name_ast = ast->function_call.function_variable;
    massert(function_name_ast->kind == ast_variable, str("expected ast_variable"));
    String function_name = function_name_ast->variable.name;
    String* namespaces = function_name_ast->variable.namespaces;
    u64 namespaces_count = function_name_ast->variable.namespaces_count;

    if (namespaces_count != 0) {
        log_error_ast(ast, "expected no namespaces when calling int_type");
        return (Expression){0};
    }
    massert(string_equal(function_name, str("int_type")), str("expected int_type"));

    if (ast->function_call.parameters_count != 1) {
        log_error_ast(ast, "expected 1 parameter to int_type");
        return (Expression){0};
    }

    Ast* parameter_ast = &ast->function_call.parameters[0];
    Expression parameter = sem_expression_parse(parameter_ast);
    parameter = sem_get_value_if(&parameter);
    if (parameter.kind == expression_invalid) return (Expression){0};
    switch (parameter.type.kind) {
        case type_int_literal: {
            void* value = sem_evaluate_expression(&parameter);
            if (value == NULL) return (Expression){0};
            i64 bits = *(i64*)value;
            expr.int_type.bits = bits;
            break;
        }
        case type_int: {
            Type i64_type = sem_int_type(64);
            parameter = sem_cast_without_allocator(&parameter, &i64_type);
            void* value = sem_evaluate_expression(&parameter);
            if (value == NULL) return (Expression){0};
            i64 bits = *(i64*)value;
            expr.int_type.bits = bits;
            break;
        }
        case type_uint: {
            Type i64_type = sem_int_type(64);
            parameter = sem_cast_without_allocator(&parameter, &i64_type);
            void* value = sem_evaluate_expression(&parameter);
            if (value == NULL) return (Expression){0};
            i64 bits = *(i64*)value;
            expr.int_type.bits = bits;
            break;
        }
        default: {
            log_error_ast(ast, "expected int, uint, or int_literal as parameter to int_type");
            return (Expression){0};
        }
    }
    expr.int_type.expr = cap_alloc(sizeof(Expression));
    *expr.int_type.expr = parameter;
    return expr;
}

Expression sem_expression_int_type(Ast* ast) {
    return _sem_expression_int_uint_type(ast, false);
}

Expression sem_expression_uint_type(Ast* ast) {
    return _sem_expression_int_uint_type(ast, true);
}

Expression sem_expression_function_call_parse(Ast* ast) {
    massert(ast->kind == ast_function_call, str("expected ast_function_call"));

    Ast* function_name_ast = ast->function_call.function_variable;
    massert(function_name_ast->kind == ast_variable, str("expected ast_variable"));
    String function_name = function_name_ast->variable.name;
    String* namespaces = function_name_ast->variable.namespaces;
    u64 namespaces_count = function_name_ast->variable.namespaces_count;

    if (string_equal(function_name, str("int_type"))) {
        return sem_expression_int_type(ast);
    }
    if (string_equal(function_name, str("uint_type"))) {
        return sem_expression_uint_type(ast);
    }

    if (string_equal(function_name, str("alloc"))) {
        return sem_expression_alloc(ast);
    }

    Ast* parameters_ast = ast->function_call.parameters;
    u64 parameter_count = ast->function_call.parameters_count;

    Expression* parameters = cap_alloc(parameter_count * sizeof(Expression));
    for (u64 i = 0; i < parameter_count; i++) {
        Ast* parameter_ast = &parameters_ast[i];
        Expression parameter = sem_expression_parse(parameter_ast);
        if (parameter.kind == expression_invalid) return (Expression){0};
        parameters[i] = parameter;
    }

    Type* return_types = NULL;
    u64 return_types_count = 0;
    Function* function = sem_find_function(function_name, namespaces, namespaces_count, &parameters, &parameter_count, &return_types, &return_types_count, ast);
    if (function == NULL) return (Expression){0};
    return sem_function_call(function, parameters, return_types, return_types_count, parameter_count, ast);
}

Expression sem_expression_struct_parse(Ast* ast) {
    massert(ast->kind == ast_struct, str("expected ast_struct"));
    Expression expr = {0};
    expr.kind = expression_struct;
    expr.ast = ast;
    expr.type = sem_type_type();

    Expression* type = cap_alloc(sizeof(Type) * ast->struct_.fields_count);
    String* field_names = cap_alloc(sizeof(String) * ast->struct_.fields_count);
    for (u64 i = 0; i < ast->struct_.fields_count; i++) {
        Ast* field_ast = &ast->struct_.fields[i];
        massert(field_ast->kind == ast_struct_field, str("expected ast_struct_field"));
        Ast* field_type_ast = field_ast->struct_field.type;
        String field_name = field_ast->struct_field.name;
        Expression field_type = sem_expression_parse(field_type_ast);
        field_type = sem_get_value_if(&field_type);
        if (field_type.kind == expression_invalid) return (Expression){0};
        if (field_type.type.kind != type_type) {
            log_error_ast(field_type_ast, "expected type");
            return (Expression){0};
        }
        type[i] = field_type;
        field_names[i] = field_name;
    }
    expr.struct_.field_types = type;
    expr.struct_.field_names = field_names;
    expr.struct_.field_count = ast->struct_.fields_count;
    return expr;
}

Expression sem_expression_struct_field_access_parse(Expression* lhs, String field_name, Ast* ast) {
    Type* lhs_type = &lhs->type;
    massert(lhs_type->kind == type_struct || lhs_type->kind == type_reference, str("expected struct or reference"));
    Type struct_type;
    if (lhs_type->kind == type_reference) {
        struct_type = sem_type_dereference(lhs_type);
    } else {
        struct_type = *lhs_type;
    }

    Expression expr = {0};
    expr.kind = expression_struct_field_access;
    expr.ast = ast;
    expr.struct_field_access.struct_value = cap_alloc(sizeof(Expression));
    *expr.struct_field_access.struct_value = *lhs;

    u64 field_index = UINT64_MAX;
    Type* field_type = NULL;
    for (u64 i = 0; i < struct_type.struct_.field_count; i++) {
        String field_name = struct_type.struct_.field_names[i];
        Type* field_type_ = struct_type.struct_.field_types[i];
        if (string_equal(field_name, field_name)) {
            field_type = field_type_;
            field_index = i;
            break;
        }
    }
    if (field_index == UINT64_MAX) {
        log_error_ast(ast, "could not find field %.*s", str_info(field_name));
        return (Expression){0};
    }

    // TODO: figure out allocator
    Type new_type = sem_type_new_allocator_ids(field_type);
    if (lhs_type->kind == type_reference) {
        new_type = sem_type_reference(&new_type, ast);
    }

    expr.type = new_type;
    expr.struct_field_access.field_index = field_index;
    return expr;
}

Expression sem_expression_field_access_parse(Ast* ast) {
    massert(ast->kind == ast_field_access, str("Expected ast_field_access"));
    Expression value = sem_expression_parse(ast->field_access.value);
    String field_name = ast->field_access.field_name;

    Type underlying_type;
    if (value.type.kind == type_reference) {
        underlying_type = sem_type_dereference(&value.type);
    } else {
        underlying_type = sem_type_invalid();
    }

    if (value.type.kind == type_struct || underlying_type.kind == type_struct) {
        return sem_expression_struct_field_access_parse(&value, field_name, ast);
    }
    massert(false, str("not implemented"));
    return (Expression){0};
}

Expression sem_expression_parse_with_variable_declaration(Ast* ast) {
    switch (ast->kind) {
        case ast_field_access: {
            return sem_expression_field_access_parse(ast);
        }
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
        var->compile_time_value = NULL;
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
    if (sem_type_equal(&expr_type, type, false, true)) return true;
    if (expr_type.kind == type_int_literal && type->kind == type_int) return true;
    if (expr_type.kind == type_int_literal && type->kind == type_uint) return true;
    if (expr_type.kind == type_int_literal && type->kind == type_float) return true;
    if (expr_type.kind == type_float_literal && type->kind == type_float) return true;
    if (expr_type.kind == type_pointer && type->kind == type_pointer) {
        Type underlying_type_a = sem_type_underlying_type(&expr_type);
        Type underlying_type_b = sem_type_underlying_type(type);
        if (underlying_type_a.kind == type_void || underlying_type_b.kind == type_void) return true;
    }
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
            sem_connect_allocator_ids(et.pointer.underlying_type->allocator_id, tt.pointer.underlying_type->allocator_id, expr_in->ast);
            et = sem_type_underlying_type(&et);
            tt = sem_type_underlying_type(&tt);
        }
        massert(et.kind != type_pointer && tt.kind != type_pointer, str("expected no pointers"));
        sem_connect_allocator_ids(et.allocator_id, tt.allocator_id, expr_in->ast);
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
    if (sem_type_equal(type, &expr->type, true, true)) return *expr;
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
    Type* return_type_ptr = cap_alloc(sizeof(Type));
    *return_type_ptr = return_type;
    Type** return_type_ptr_ptr = cap_alloc(sizeof(Type*));
    *return_type_ptr_ptr = return_type_ptr;
    Expression return_type_expr = sem_compile_time_value_expression(return_type_ptr_ptr, sem_type_type(), ast);
    Expression* return_type_expr_ptr = cap_alloc(sizeof(Expression));
    *return_type_expr_ptr = return_type_expr;
    String name = ast->program.name;
    Scope* parse_scope = cap_alloc(sizeof(Scope));
    Function function = sem_create_function_internal(name, ast);
    program.function = function;

    Function_Implementation* implmentation = cap_alloc(sizeof(Function_Implementation));
    implmentation->parameter_count = 0;
    implmentation->parameters = NULL;
    implmentation->is_complete = true;
    implmentation->body.parent = function.scope_created_in;
    implmentation->return_types_count = 1;
    implmentation->return_types = cap_alloc(sizeof(Type) * implmentation->return_types_count);
    implmentation->return_types[0] = sem_int_type(32);
    implmentation->lost_compile_time_evaluatable_at = NULL;
    ptr_append(function.internal.implementations, function.internal.implementations_count, function.internal.implementations_capacity, implmentation);

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
