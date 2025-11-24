#include <llvm-c/Core.h>

#include "cap.h"
#include "cap/semantic.h"

LLVM_Context llvm_context = {0};

LLVMTypeRef llvm_get_type(Type* type) {
    if (type->ptr_count > 0) {
        type->ptr_count -= 1;
        LLVMTypeRef ptr_type = llvm_get_type(type);
        type->ptr_count += 1;
        return LLVMPointerType(ptr_type, 0);
    }
    if (type->is_ref) {
        type->is_ref = false;
        LLVMTypeRef ref_type = llvm_get_type(type);
        type->is_ref = true;
        return LLVMPointerType(ref_type, 0);
    }
    switch (type->base->kind) {
        case type_struct: {
            LLVMTypeRef struct_field_types[type->base->struct_.fields.count];
            for (u32 i = 0; i < type->base->struct_.fields.count; i++) {
                Struct_Field* field = &type->base->struct_.fields.data[i];
                struct_field_types[i] = llvm_get_type(&field->type);
            }
            return LLVMStructType(struct_field_types, type->base->struct_.fields.count, false);
        }
        case type_int:
            return LLVMIntType(type->base->number_bit_size);
        case type_float:
            switch (type->base->number_bit_size) {
                case 32:
                    return LLVMFloatType();
                case 64:
                    return LLVMDoubleType();
                default:
                    massert(false, "should have found a valid float type");
                    return NULL;
            }
        case type_uint:
            return LLVMIntType(type->base->number_bit_size);
        case type_void:
            return LLVMVoidType();
        case type_type:
            return NULL;
        case type_const_int:
        case type_const_float:
            return NULL;
        case type_ptr:
        case type_ref:
            massert(false, "ptr and ref types should have been handled by the time we get here");
            return NULL;
        case type_invalid:
            massert(false, "should have a valid type");
            return NULL;
    }
}

void llvm_setup_program_context() {
    llvm_context.module = LLVMModuleCreateWithName("cap");

    LLVM_Function malloc_function = {0};
    LLVMTypeRef return_type = LLVMPointerType(LLVMIntType(8), 0);
    LLVMTypeRef param_types[] = {LLVMIntType(64)};
    LLVMTypeRef malloc_type = LLVMFunctionType(return_type, param_types, 1, 0);
    LLVMValueRef malloc_func = LLVMAddFunction(llvm_context.module, "malloc", malloc_type);
    malloc_function.function_value = malloc_func;
    malloc_function.function_type = malloc_type;
    malloc_function.templated_function = NULL;
    llvm_context.malloc_function = malloc_function;
}
void llvm_cleanup_program_context() {
    llvm_context.llvm_functions.count = 0;
    LLVMDisposeModule(llvm_context.module);
}

LLVMValueRef llvm_variable_to_llvm(LLVM_Scope* scope, Variable* variable) {
    for (u32 i = 0; i < scope->variables.count; i++) {
        LLVM_Variable_Pair* pair = LLVM_Variable_Pair_List_get(&scope->variables, i);
        Variable* pair_variable = pair->variable;
        if (pair_variable == variable) {
            return pair->value;
        }
    }
    if (scope->parent != NULL) {
        return llvm_variable_to_llvm(scope->parent, variable);
    }
    massert(false, "should have found a variable");
    return NULL;
}

void llvm_store_variable_llvm_value(LLVM_Scope* scope, Variable* variable, LLVMValueRef value) {
    LLVM_Variable_Pair pair = {0};
    pair.variable = variable;
    pair.value = value;
    LLVM_Variable_Pair_List_add(&scope->variables, &pair);
}

LLVM_Scope llvm_compile_scope(Scope* scope, LLVM_Function* function, LLVM_Scope* parent) {
    LLVM_Scope llvm_scope = {0};
    llvm_scope.scope = scope;
    llvm_scope.parent = parent;

    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(function->function_value, "entry");
    llvm_scope.block = entry_block;
    LLVMPositionBuilderAtEnd(llvm_context.builder, entry_block);

    for (u32 i = 0; i < scope->variables.count; i++) {
        Variable* variable = *Variable_Ptr_List_get(&scope->variables, i);
        Allocator* allocator = sem_allocator_get(&function->allocator_connection_map, variable->type.ref_allocator_id);
        Type deref_type = sem_dereference_type(&variable->type);
        LLVMTypeRef llvm_type = llvm_get_type(&deref_type);
        LLVMValueRef alloca = LLVMBuildAlloca(llvm_context.builder, llvm_type, variable->name);
        LLVMValueRef zero = LLVMConstNull(llvm_type);
        LLVMBuildStore(llvm_context.builder, zero, alloca);
        llvm_store_variable_llvm_value(&llvm_scope, variable, alloca);
    }

    for (u32 i = 0; i < scope->statements.count; i++) {
        Statement* statement = Statement_List_get(&scope->statements, i);
        bool exit_scope = llvm_build_statement(statement, &llvm_scope, function);
        if (exit_scope) {
            llvm_scope.has_exit = true;
            break;
        }
    }

    return llvm_scope;
}

LLVMValueRef llvm_build_expression_int(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    massert(expression->kind == expression_int, "should have found an int");
    return NULL;
}

LLVMValueRef llvm_build_expression_float(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    massert(expression->kind == expression_float, "should have found a float");
    return NULL;
}

LLVMValueRef llvm_build_expression_variable(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    massert(expression->kind == expression_variable, "should have found a variable");
    Variable* variable = sem_scope_get_variable(scope->scope, expression->variable.variable->name);
    return llvm_variable_to_llvm(scope, variable);
}

LLVMValueRef llvm_build_expression_struct_access(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    LLVMValueRef struct_expr = llvm_build_expression(expression->struct_access.expression, scope, function);
    u32 field_index = expression->struct_access.field_index;

    Type* type = &expression->type;
    Type struct_type = expression->struct_access.expression->type;
    if (struct_type.is_ref) struct_type = sem_dereference_type(&struct_type);
    LLVMTypeRef llvm_type = llvm_get_type(&struct_type);
    if (type->is_ref) {
        return LLVMBuildStructGEP2(llvm_context.builder, llvm_type, struct_expr, field_index, "");
    } else {
        return LLVMBuildExtractValue(llvm_context.builder, struct_expr, field_index, "");
    }
}

LLVMValueRef llvm_build_expression(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    switch (expression->kind) {
        case expression_struct_access: {
            return llvm_build_expression_struct_access(expression, scope, function);
        }
        case expression_function_call: {
            return llvm_build_expression_function_call(expression, scope, function);
        }
        case expression_alloc: {
            return llvm_build_expression_alloc(expression, scope, function);
        }
        case expression_int: {
            return llvm_build_expression_int(expression, scope, function);
        }
        case expression_float: {
            return llvm_build_expression_float(expression, scope, function);
        }
        case expression_variable: {
            return llvm_build_expression_variable(expression, scope, function);
        }
        case expression_cast: {
            return llvm_build_expression_cast(expression, scope, function);
        }
        case expression_ptr: {
            return llvm_build_expression_ptr(expression, scope, function);
        }
        case expression_get: {
            return llvm_build_expression_get(expression, scope, function);
        }
        case expression_type: {
            return llvm_build_expression_type(expression, scope, function);
        }
        case expression_type_size: {
            return llvm_build_expression_type_size(expression, scope, function);
        }
        case expression_type_align: {
            return llvm_build_expression_type_align(expression, scope, function);
        }
        case expression_invalid: {
            massert(false, "should never happen");
            break;
        }
    }
}

LLVMValueRef llvm_build_expression_type(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    return NULL;
}

LLVMValueRef llvm_build_expression_type_size(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    return NULL;
}

LLVMValueRef llvm_build_expression_type_align(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    return NULL;
}

LLVMValueRef llvm_build_expression_function_call(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    massert(expression->kind == expression_function_call, "should have found a function call");
    LLVMValueRef parameter_values[expression->function_call.parameters.count];
    for (u32 i = 0; i < expression->function_call.parameters.count; i++) {
        Expression* parameter = Expression_List_get(&expression->function_call.parameters, i);
        LLVMValueRef parameter_value = llvm_build_expression(parameter, scope, function);
        parameter_values[i] = parameter_value;
    }

    Templated_Function* templated_function = expression->function_call.templated_function;
    LLVM_Function* llvm_function = llvm_get_function(templated_function, function, &expression->function_call.parameters, &expression->type);

    LLVMValueRef return_value = LLVMBuildCall2(llvm_context.builder, llvm_function->function_type, llvm_function->function_value, parameter_values, expression->function_call.parameters.count, "");
    return return_value;
}

LLVMValueRef llvm_build_expression_alloc(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    massert(expression->kind == expression_alloc, "should have found an alloc");
    LLVMValueRef size_in_bytes_value;
    LLVMValueRef alignment_value;
    LLVMTypeRef llvm_type;
    LLVMValueRef count_value = NULL;

    massert(expression->type.ptr_count > 0, "should have found a pointer");
    u32 expression_allocator_id = expression->type.ptr_allocator_ids[expression->type.ptr_count - 1];
    Allocator* allocator = sem_allocator_get(&function->allocator_connection_map, expression_allocator_id);

    if (expression->alloc.is_type_alloc) {
        Type* type_to_alloc = &expression->alloc.type_or_count->expression_type.type;
        llvm_type = llvm_get_type(type_to_alloc);
        u64 size_in_bytes = LLVMABISizeOfType(llvm_context.data_layout, llvm_type);
        size_in_bytes_value = LLVMConstInt(LLVMInt64Type(), size_in_bytes, false);
        int alignment = LLVMABIAlignmentOfType(llvm_context.data_layout, llvm_type);
        alignment_value = LLVMConstInt(LLVMInt32Type(), alignment, true);
        if (expression->alloc.count_or_allignment != NULL) {
            Expression* count_expression = expression->alloc.count_or_allignment;
            count_value = llvm_build_expression(count_expression, scope, function);
            size_in_bytes_value = LLVMBuildMul(llvm_context.builder, size_in_bytes_value, count_value, "size_in_bytes");
        }
    } else {
        size_in_bytes_value = llvm_build_expression(expression->alloc.type_or_count, scope, function);
        alignment_value = llvm_build_expression(expression->alloc.count_or_allignment, scope, function);
    }

    if (sem_allocator_are_the_same(allocator, &STACK_ALLOCATOR)) {
        if (expression->alloc.is_type_alloc) {
            if (count_value != NULL) {
                return LLVMBuildArrayAlloca(llvm_context.builder, llvm_type, count_value, "stack_alloc");
            } else {
                return LLVMBuildAlloca(llvm_context.builder, llvm_type, "stack_alloc");
            }
        } else {
            LLVMTypeRef i8_llvm = LLVMIntType(8);
            LLVMValueRef alignment_value_u64 = LLVMBuildZExt(llvm_context.builder, alignment_value, LLVMIntType(64), "");
            LLVMValueRef size_plus_align = LLVMBuildAdd(
                llvm_context.builder,
                size_in_bytes_value,
                LLVMBuildSub(llvm_context.builder, alignment_value_u64, LLVMConstInt(LLVMIntType(64), 1, 0), ""),
                "");
            LLVMValueRef raw_alloc = LLVMBuildArrayAlloca(llvm_context.builder, i8_llvm, size_plus_align, "stack_alloc_raw");
            // Compute aligned pointer: (raw_ptr + (alignment - 1)) & ~(alignment - 1)
            LLVMValueRef raw_ptr_int = LLVMBuildPtrToInt(llvm_context.builder, raw_alloc, LLVMIntType(64), "");
            LLVMValueRef align_minus_1 = LLVMBuildSub(llvm_context.builder, alignment_value_u64, LLVMConstInt(LLVMIntType(64), 1, 0), "");
            LLVMValueRef add = LLVMBuildAdd(llvm_context.builder, raw_ptr_int, align_minus_1, "");
            LLVMValueRef mask = LLVMBuildNot(llvm_context.builder, align_minus_1, "");
            LLVMValueRef aligned_ptr_int = LLVMBuildAnd(llvm_context.builder, add, mask, "");
            LLVMValueRef aligned_alloc = LLVMBuildIntToPtr(llvm_context.builder, aligned_ptr_int, LLVMPointerType(i8_llvm, 0), "stack_alloc");
            return aligned_alloc;
        }
    }
    if (sem_allocator_are_the_same(allocator, &UNSPECIFIED_ALLOCATOR)) {
        return LLVMBuildCall2(llvm_context.builder, llvm_context.malloc_function.function_type, llvm_context.malloc_function.function_value, &size_in_bytes_value, 1, "");
    }

    (void)alignment_value;
    massert(false, "not implemented");
    return NULL;
}

LLVMValueRef llvm_build_expression_ptr(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    massert(expression->kind == expression_ptr, "should have found a ptr");
    Expression* underlying_expression = expression->ptr.expression;
    LLVMValueRef underlying_value = llvm_build_expression(underlying_expression, scope, function);
    return underlying_value;
}

LLVMValueRef llvm_build_expression_get(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    massert(expression->kind == expression_get, "should have found a get");
    Type* expr_type = &expression->type;
    Expression* underlying_expression = expression->get.expression;
    LLVMValueRef underlying_value = llvm_build_expression(underlying_expression, scope, function);
    return underlying_value;
}

LLVMValueRef llvm_build_expression_cast(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    massert(expression->kind == expression_cast, "should have found a cast");
    Expression* from_expression = expression->cast.expression;
    Type from_type = from_expression->type;
    Type to_type = expression->type;
    LLVMValueRef from_value = llvm_build_expression(from_expression, scope, function);

    if (sem_type_is_equal_without_allocator_id(&from_type, &to_type)) {
        return from_value;
    }

    if (from_type.is_ref && !to_type.is_ref) {
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        LLVMValueRef deref_value = LLVMBuildLoad2(llvm_context.builder, to_type_llvm_type, from_value, "");
        return deref_value;
    }

    Type_Kind from_kind = sem_get_type_kind(&from_type);
    Type_Kind to_kind = sem_get_type_kind(&to_type);
    if (from_kind == type_int && to_kind == type_int) {
        u64 from_bit_size = from_type.base->number_bit_size;
        u64 to_bit_size = to_type.base->number_bit_size;
        if (from_bit_size < to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildSExt(llvm_context.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        } else if (from_bit_size > to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildTrunc(llvm_context.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        }
        return from_value;
    }

    if (from_kind == type_uint && to_kind == type_uint) {
        u64 from_bit_size = from_type.base->number_bit_size;
        u64 to_bit_size = to_type.base->number_bit_size;
        if (from_bit_size < to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildZExt(llvm_context.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        } else if (from_bit_size > to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildTrunc(llvm_context.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        }
        return from_value;
    }

    if (from_kind == type_float && to_kind == type_float) {
        LLVMTypeRef from_llvm_type = llvm_get_type(&from_type);
        LLVMTypeRef to_llvm_type = llvm_get_type(&to_type);
        u64 from_bit_size = from_type.base->number_bit_size;
        u64 to_bit_size = to_type.base->number_bit_size;

        if (from_bit_size < to_bit_size) {
            return LLVMBuildFPExt(llvm_context.builder, from_value, to_llvm_type, "");
        } else if (from_bit_size > to_bit_size) {
            return LLVMBuildFPTrunc(llvm_context.builder, from_value, to_llvm_type, "");
        }
        return from_value;
    }

    if (from_kind == type_int && to_kind == type_uint) {
        u64 from_bit_size = from_type.base->number_bit_size;
        u64 to_bit_size = to_type.base->number_bit_size;
        if (from_bit_size < to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildZExt(llvm_context.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        } else if (from_bit_size > to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildTrunc(llvm_context.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        }
        return from_value;
    }

    if (from_kind == type_uint && to_kind == type_int) {
        u64 from_bit_size = from_type.base->number_bit_size;
        u64 to_bit_size = to_type.base->number_bit_size;
        if (from_bit_size < to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildZExt(llvm_context.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        } else if (from_bit_size > to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildTrunc(llvm_context.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        }
        return from_value;
    }

    if (from_kind == type_int && to_kind == type_float) {
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        LLVMValueRef cast_value = LLVMBuildSIToFP(llvm_context.builder, from_value, to_type_llvm_type, "");
        return cast_value;
    }

    if (from_kind == type_uint && to_kind == type_float) {
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        LLVMValueRef cast_value = LLVMBuildUIToFP(llvm_context.builder, from_value, to_type_llvm_type, "");
        return cast_value;
    }

    if (from_kind == type_const_int && to_kind == type_int) {
        char* value = llvm_evaluate_const_int(from_expression);
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        return LLVMConstIntOfString(to_type_llvm_type, value, 10);
    }

    if (from_kind == type_const_int && to_kind == type_uint) {
        char* value = llvm_evaluate_const_int(from_expression);
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        return LLVMConstIntOfString(to_type_llvm_type, value, 10);
    }

    if (from_kind == type_const_int && to_kind == type_float) {
        char* value = llvm_evaluate_const_int(from_expression);
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        return LLVMConstRealOfString(to_type_llvm_type, value);
    }

    if (from_kind == type_const_float && to_kind == type_float) {
        double value = llvm_evaluate_const_float(from_expression);
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        return LLVMConstReal(to_type_llvm_type, value);
    }

    if (from_kind == type_const_float && to_kind == type_int) {
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        double value = llvm_evaluate_const_float(from_expression);
        i64 value_i64 = (i64)value;
        return LLVMConstInt(to_type_llvm_type, value_i64, true);
    }

    if (from_kind == type_const_float && to_kind == type_uint) {
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        double value = llvm_evaluate_const_float(from_expression);
        u64 value_u64 = (u64)value;
        return LLVMConstInt(to_type_llvm_type, value_u64, false);
    }

    if (from_kind == type_ptr && to_kind == type_ptr) {
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        return from_value;
    }

    massert(false, "should have found a valid cast");
    return NULL;
}

bool llvm_build_statement_assignment(Statement* statement, LLVM_Scope* scope, LLVM_Function* function) {
    massert(statement->type == statement_assignment, "should have found an assignment");
    LLVMValueRef assignee = llvm_build_expression(&statement->assignment.assignee, scope, function);
    massert(LLVMGetTypeKind(LLVMTypeOf(assignee)) == LLVMPointerTypeKind, "should have found a pointer");

    massert(statement->assignment.assignee.type.is_ref, "should have found a ref to assign to");
    LLVMValueRef value = llvm_build_expression(&statement->assignment.value, scope, function);

    LLVMBuildStore(llvm_context.builder, value, assignee);
    return false;
}

bool llvm_build_statement_return(Statement* statement, LLVM_Scope* scope, LLVM_Function* function) {
    massert(statement->type == statement_return, "should have found a return");
    if (statement->return_.value.kind == expression_invalid) {
        LLVMBuildRetVoid(llvm_context.builder);
        return true;
    }
    LLVMValueRef value = llvm_build_expression(&statement->return_.value, scope, function);
    LLVMBuildRet(llvm_context.builder, value);
    return true;
}

bool llvm_build_statement_expression(Statement* statement, LLVM_Scope* scope, LLVM_Function* function) {
    massert(statement->type == statement_expression, "should have found an expression");
    llvm_build_expression(&statement->expression.value, scope, function);
    return false;
}

bool llvm_build_statement_variable_declaration(Statement* statement, LLVM_Scope* scope, LLVM_Function* function) {
    massert(statement->type == statement_variable_declaration, "should have found a variable declaration");
    if (statement->variable_declaration.assignment != NULL) {
        llvm_build_statement_assignment(statement->variable_declaration.assignment, scope, function);
    }
    return false;
}

bool llvm_build_statement(Statement* statement, LLVM_Scope* scope, LLVM_Function* function) {
    switch (statement->type) {
        case statement_assignment: {
            return llvm_build_statement_assignment(statement, scope, function);
        }
        case statement_return: {
            return llvm_build_statement_return(statement, scope, function);
        }
        case statement_expression: {
            return llvm_build_statement_expression(statement, scope, function);
        }
        case statement_variable_declaration: {
            return llvm_build_statement_variable_declaration(statement, scope, function);
        }
        case statement_invalid: {
            massert(false, "should never happen");
            break;
        }
    }
}

static bool llvm_allocator_id_match_for_allocator_function_call(u32 function_allocator_id, u32 parameter_allocator_id, LLVM_Function* function, LLVM_Function* parameter_function) {
    Allocator* function_allocator = sem_allocator_get(&function->allocator_connection_map, function_allocator_id);
    massert(function_allocator->is_function_value, "should have found a function value");
    if (!function_allocator->used_for_alloc_or_free) return true;

    Allocator* parameter_allocator = sem_allocator_get(&parameter_function->allocator_connection_map, parameter_allocator_id);

    if (sem_allocator_are_the_same(function_allocator, parameter_allocator)) return true;

    Variable* function_allocator_variable = function_allocator->variable;
    // handle special allocators like stack for which variable is not a valid pointer
    if ((u64)function_allocator_variable <= 255) return false;

    Variable* parameter_allocator_variable = parameter_allocator->variable;
    // handle special allocators like stack for which variable is not a valid pointer
    if ((u64)parameter_allocator_variable <= 255) return false;

    Type* function_allocator_type = &function_allocator_variable->type;
    Type* parameter_allocator_type = &parameter_allocator_variable->type;

    if (!sem_type_is_equal_without_allocator_id(function_allocator_type, parameter_allocator_type)) return false;
    return true;
}

static bool llvm_type_match_for_allocator_function_call(Type* function_type, Type* parameter_type, LLVM_Function* function, LLVM_Function* parameter_function, bool is_return_value) {
    massert(sem_base_allocator_matters(function_type) == sem_base_allocator_matters(parameter_type), "should have found a valid type");
    if (sem_base_allocator_matters(function_type)) {
        u32 function_allocator_id = function_type->base_allocator_id;
        u32 parameter_allocator_id = parameter_type->base_allocator_id;
        if (!llvm_allocator_id_match_for_allocator_function_call(function_allocator_id, parameter_allocator_id, function, parameter_function)) return false;
    }

    massert(function_type->ptr_count == parameter_type->ptr_count, "should have found a valid type");
    massert(function_type->is_ref == parameter_type->is_ref, "should have found a valid type");
    massert(!function_type->is_ref, "should have found a valid type");
    u64 loop_size = function_type->ptr_count;
    if (!is_return_value && loop_size > 0) loop_size -= 1;
    for (u32 i = 0; i < loop_size; i++) {
        u32 function_allocator_id = function_type->ptr_allocator_ids[i];
        u32 parameter_allocator_id = parameter_type->ptr_allocator_ids[i];
        if (!llvm_allocator_id_match_for_allocator_function_call(function_allocator_id, parameter_allocator_id, function, parameter_function)) return false;
    }

    return true;
}

static void llvm_add_id_allocator_llvm_type_to_list(u32 function_allocator_id, u32 parameter_allocator_id, LLVM_Function* function, LLVM_Function* parameter_function, bool is_return_value, LLVMTypeRef_List* list_to_add_to) {
    Allocator* function_allocator = sem_allocator_get(&function->allocator_connection_map, function_allocator_id);
    if (function_allocator->is_function_value) {
        Allocator* parameter_allocator = sem_allocator_get(&parameter_function->allocator_connection_map, parameter_allocator_id);
        Variable* function_allocator_variable = function_allocator->variable;
        Variable* parameter_allocator_variable = parameter_allocator->variable;
        if ((u64)parameter_allocator_variable <= 255) {
            Allocator new_allocator = {0};
            new_allocator.variable = parameter_allocator_variable;
            new_allocator.used_for_alloc_or_free = true;
            new_allocator.is_function_value = true;
            sem_set_allocator(&function->allocator_connection_map, function_allocator_id, &new_allocator);
        } else {
            Allocator new_allocator = {0};
            Variable* new_variable = alloc(sizeof(Variable));
            *new_variable = *parameter_allocator_variable;
            new_allocator.variable = new_variable;
            new_allocator.used_for_alloc_or_free = true;
            new_allocator.is_function_value = true;
            sem_set_allocator(&function->allocator_connection_map, function_allocator_id, &new_allocator);
            Type* parameter_variable_type = &parameter_allocator_variable->type;
            LLVMTypeRef llvm_parameter_variable_type = llvm_get_type(parameter_variable_type);
            if (list_to_add_to != NULL) LLVMTypeRef_List_add(list_to_add_to, &llvm_parameter_variable_type);
        }
    }
}

static void llvm_add_type_allocator_llvm_type_to_list(Type* function_type, Type* parameter_type, LLVM_Function* function, LLVM_Function* parameter_function, bool is_return_value, LLVMTypeRef_List* list_to_add_to) {
    massert(sem_base_allocator_matters(function_type) == sem_base_allocator_matters(parameter_type), "should have found a valid type");
    if (sem_base_allocator_matters(function_type)) {
        u32 function_allocator_id = function_type->base_allocator_id;
        u32 parameter_allocator_id = parameter_type->base_allocator_id;
        llvm_add_id_allocator_llvm_type_to_list(function_allocator_id, parameter_allocator_id, function, parameter_function, is_return_value, list_to_add_to);
    }

    u64 loop_size = function_type->ptr_count;
    if (!is_return_value && loop_size > 0) loop_size -= 1;
    massert(function_type->ptr_count == parameter_type->ptr_count, "should have found a valid type");
    massert(function_type->is_ref == parameter_type->is_ref, "should have found a valid type");
    massert(!function_type->is_ref, "should have found a valid type");
    for (u32 i = 0; i < loop_size; i++) {
        u32 function_allocator_id = function_type->ptr_allocator_ids[i];
        u32 parameter_allocator_id = parameter_type->ptr_allocator_ids[i];
        llvm_add_id_allocator_llvm_type_to_list(function_allocator_id, parameter_allocator_id, function, parameter_function, is_return_value, list_to_add_to);
    }
}

LLVM_Function* llvm_get_function(Templated_Function* templated_function, LLVM_Function* function_getting_this, Expression_List* parameters, Type* return_type) {
    // check if we have already compiled this function
    for (u32 i = 0; i < llvm_context.llvm_functions.count; i++) {
        LLVM_Function* function = LLVM_Function_List_get(&llvm_context.llvm_functions, i);

        if (function->templated_function != templated_function) continue;

        Type* function_return_type = &function->templated_function->return_type;
        if (!llvm_type_match_for_allocator_function_call(function_return_type, return_type, function, function_getting_this, true)) continue;

        bool allocators_match = true;
        for (u32 j = 0; j < function->templated_function->parameter_scope.variables.count; j++) {
            Variable* function_variable = *Variable_Ptr_List_get(&function->templated_function->parameter_scope.variables, j);
            Expression* parameter = Expression_List_get(parameters, j);

            Type function_type = function_variable->type;
            function_type = sem_dereference_type(&function_type);

            Type* parameter_type = &parameter->type;
            if (!llvm_type_match_for_allocator_function_call(&function_type, parameter_type, function, function_getting_this, false)) {
                allocators_match = false;
                break;
            }
        }
        if (!allocators_match) continue;

        return function;
    }

    LLVM_Function function = {0};
    function.allocator_connection_map = sem_copy_allocator_connection_map(&templated_function->allocator_connection_map);

    LLVMTypeRef llvm_return_type = llvm_get_type(&templated_function->return_type);
    LLVMTypeRef_List llvm_parameter_types = {0};
    for (u32 i = 0; i < templated_function->parameter_scope.variables.count; i++) {
        Variable* variable = *Variable_Ptr_List_get(&templated_function->parameter_scope.variables, i);
        Type* type = &variable->type;
        Type deref_type = sem_dereference_type(type);
        LLVMTypeRef llvm_type = llvm_get_type(&deref_type);
        LLVMTypeRef_List_add(&llvm_parameter_types, &llvm_type);
    }

    for (u32 i = 0; i < templated_function->parameter_scope.variables.count; i++) {
        Variable* variable = *Variable_Ptr_List_get(&templated_function->parameter_scope.variables, i);
        Type type = variable->type;
        type = sem_dereference_type(&type);
        Expression* parameter = Expression_List_get(parameters, i);
        Type* expression_type = &parameter->type;
        llvm_add_type_allocator_llvm_type_to_list(&type, expression_type, &function, function_getting_this, false, &llvm_parameter_types);
    }
    llvm_add_type_allocator_llvm_type_to_list(&templated_function->return_type, return_type, &function, function_getting_this, true, NULL);

    LLVMTypeRef function_type = LLVMFunctionType(llvm_return_type, llvm_parameter_types.data, llvm_parameter_types.count, false);
    LLVMValueRef function_value = LLVMAddFunction(llvm_context.module, templated_function->function->name, function_type);

    function.templated_function = templated_function;
    function.function_value = function_value;
    function.function_type = function_type;

    if (templated_function->body.ast == NULL) {
        LLVM_Function_List_add(&llvm_context.llvm_functions, &function);
        return LLVM_Function_List_get(&llvm_context.llvm_functions, llvm_context.llvm_functions.count - 1);
    }

    LLVMBasicBlockRef return_after_building_function_block = LLVMAppendBasicBlock(function_getting_this->function_value, "return_after_building_function");
    LLVMBuildBr(llvm_context.builder, return_after_building_function_block);

    LLVM_Scope llvm_scope = {0};
    llvm_scope.scope = &templated_function->parameter_scope;
    llvm_scope.has_exit = true;

    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(function_value, "entry");
    LLVMPositionBuilderAtEnd(llvm_context.builder, entry_block);
    llvm_scope.block = entry_block;

    for (u32 i = 0; i < templated_function->parameter_scope.variables.count; i++) {
        Variable* variable = *Variable_Ptr_List_get(&templated_function->parameter_scope.variables, i);
        Allocator* allocator = sem_allocator_get(&function.allocator_connection_map, variable->type.ref_allocator_id);
        LLVMValueRef parameter_value = LLVMGetParam(function_value, i);
        Type deref_type = sem_dereference_type(&variable->type);
        LLVMTypeRef llvm_type = llvm_get_type(&deref_type);
        LLVMValueRef alloca = LLVMBuildAlloca(llvm_context.builder, llvm_type, variable->name);
        LLVMBuildStore(llvm_context.builder, parameter_value, alloca);
        llvm_store_variable_llvm_value(&llvm_scope, variable, alloca);
    }

    LLVM_Scope body_scope = llvm_compile_scope(&templated_function->body, &function, &llvm_scope);
    if (!body_scope.has_exit) {
        Type_Kind return_type_kind = sem_get_type_kind(&templated_function->return_type);
        massert(return_type_kind == type_void, "should never get here");
        LLVMBuildRetVoid(llvm_context.builder);
    }

    LLVMPositionBuilderAtEnd(llvm_context.builder, entry_block);
    LLVMBuildBr(llvm_context.builder, body_scope.block);

    LLVMPositionBuilderAtEnd(llvm_context.builder, return_after_building_function_block);
    LLVM_Function_List_add(&llvm_context.llvm_functions, &function);
    return LLVM_Function_List_get(&llvm_context.llvm_functions, llvm_context.llvm_functions.count - 1);
}

void llvm_compile_program(Program* program, char* build_dir) {
    llvm_setup_program_context();

    LLVMTypeRef llvm_return_type = llvm_get_type(&program->body.return_type);
    LLVMTypeRef function_type = LLVMFunctionType(llvm_return_type, NULL, 0, false);
    LLVMValueRef function_value = LLVMAddFunction(llvm_context.module, "main", function_type);

    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(function_value, "entry");
    LLVM_Function function = {0};
    function.function_type = function_type;
    function.function_value = function_value;
    function.templated_function = program->body.templated_functions.data[0];
    function.allocator_connection_map = sem_copy_allocator_connection_map(&program->body.templated_functions.data[0]->allocator_connection_map);
    LLVMPositionBuilderAtEnd(llvm_context.builder, entry_block);

    LLVM_Scope llvm_scope = llvm_compile_scope(&function.templated_function->body, &function, NULL);
    if (!llvm_scope.has_exit) {
        LLVMValueRef zero = LLVMConstInt(llvm_get_type(&program->body.return_type), 0, 0);
        LLVMBuildRet(llvm_context.builder, zero);
        llvm_scope.has_exit = true;
    }

    LLVMPositionBuilderAtEnd(llvm_context.builder, entry_block);
    LLVMBuildBr(llvm_context.builder, llvm_scope.block);

    // ############# DONE building llvm ir #############
    char* error;
    char* ir = LLVMPrintModuleToString(llvm_context.module);
    printf("\nLLVM IR:\n%s\n\n", ir);
    LLVMDisposeMessage(ir);

    if (LLVMVerifyModule(llvm_context.module, LLVMAbortProcessAction, &error) != 0) {
        red_printf("LLVMVerifyModule failed: %s\n", error);
        LLVMDisposeMessage(error);
        return;
    } else {
        green_printf("Successfully verified module\n");
    }

    char* obj_path = alloc(8192);
    snprintf(obj_path, 8192, "%s/%s.obj", build_dir, program->name);

    if (LLVMTargetMachineEmitToFile(llvm_context.target_machine, llvm_context.module, obj_path, LLVMObjectFile, &error) != 0) {
        red_printf("Failed to emit object file: %s\n", error);
        LLVMDisposeMessage(error);
        return;
    } else {
        green_printf("Successfully compiled Tia project to: %s\n", obj_path);
    }

    char* exe_path = alloc(8192);
    snprintf(exe_path, 8192, "%s/%s.exe", build_dir, program->name);

    bool res = link_obj_to_exe(obj_path, exe_path);
    if (!res) {
        red_printf("Failed to link obj: %s to exe: %s\n", obj_path, exe_path);
    } else {
        green_printf("Successfully linked obj: %s to exe: %s\n", obj_path, exe_path);
        rainbow_printf("\n\nProgram output:\n");
        char buffer[512 * 8];
        sprintf(buffer, "./%s.exe", program->name);
        int res = system(exe_path);
        printf("\n\n");
        rainbow_printf("Program exited with code: %d\n", res);
    }

    llvm_cleanup_program_context();
}

char* llvm_evaluate_const_int(Expression* expression) {
    switch (expression->kind) {
        case expression_int: {
            return expression->int_.value;
        }
        case expression_float: {
            double value = expression->float_.value;
            i64 value_i64 = (i64)value;
            u64 number_of_digits = get_number_of_digits(value_i64);
            char* value_str = alloc(number_of_digits + 2);
            sprintf(value_str, "%lld", value_i64);
            return value_str;
        }
        case expression_type_size: {
            Type* type = &expression->type_size.type;
            LLVMTypeRef llvm_type = llvm_get_type(type);
            u64 size_in_bytes = LLVMABISizeOfType(llvm_context.data_layout, llvm_type);
            u64 number_of_digits = get_number_of_digits(size_in_bytes);
            char* value_str = alloc(number_of_digits + 2);
            sprintf(value_str, "%lld", size_in_bytes);
            return value_str;
        }
        case expression_type_align: {
            Type* type = &expression->type_align.type;
            LLVMTypeRef llvm_type = llvm_get_type(type);
            u64 alignment = LLVMABIAlignmentOfType(llvm_context.data_layout, llvm_type);
            u64 number_of_digits = get_number_of_digits(alignment);
            char* value_str = alloc(number_of_digits + 2);
            sprintf(value_str, "%lld", alignment);
            return value_str;
        }
        case expression_type:
        case expression_function_call:
        case expression_alloc:
        case expression_get:
        case expression_ptr:
        case expression_struct_access:
        case expression_variable:
        case expression_cast:
        case expression_invalid: {
            massert(false, "should never happen");
            return NULL;
        }
    }
}

double llvm_evaluate_const_float(Expression* expression) {
    switch (expression->kind) {
        case expression_int: {
            bool res;
            double value = get_string_float(expression->int_.value, &res);
            massert(res, "should have found a valid float");
            return value;
        }
        case expression_float: {
            return expression->float_.value;
        }
        case expression_type_size: {
            Type* type = &expression->type_size.type;
            LLVMTypeRef llvm_type = llvm_get_type(type);
            u64 size_in_bytes = LLVMABISizeOfType(llvm_context.data_layout, llvm_type);
            return (double)size_in_bytes;
        }
        case expression_type_align: {
            Type* type = &expression->type_align.type;
            LLVMTypeRef llvm_type = llvm_get_type(type);
            u64 alignment = LLVMABIAlignmentOfType(llvm_context.data_layout, llvm_type);
            return (double)alignment;
        }
        case expression_type:
        case expression_function_call:
        case expression_alloc:
        case expression_struct_access:
        case expression_variable:
        case expression_get:
        case expression_ptr:
        case expression_cast:
        case expression_invalid: {
            massert(false, "should never happen");
            return 0.0;
        }
    }
}
