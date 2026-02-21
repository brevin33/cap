#include "cap/llvm.h"

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Types.h>
#include <stddef.h>

#include "cap.h"
#include "cap/semantics.h"

void llvm_print_module() {
    printf("\n\n LLVM Module:\n");
    char* ir = LLVMPrintModuleToString(cap_context.llvm_info.module_info.module);
    printf("%s\n", ir);
    printf("---------\n");
    LLVMDisposeMessage(ir);
}

LLVMTypeRef llvm_get_type(Type* type) {
    switch (type->kind) {
        case type_int_literal: {
            return LLVMIntType(64);
        }
        case type_float_literal: {
            return LLVMDoubleType();
        }
        case type_type: {
            if (cap_context.is_in_semantic_analysis) return LLVMPointerType(LLVMIntType(8), 0);
            return LLVMStructType(NULL, 0, 0);
        }
        case type_int: {
            return LLVMIntType(type->int_.bits);
        }
        case type_uint: {
            return LLVMIntType(type->uint.bits);
        }
        case type_float: {
            i64 bits = type->float_.bits;
            switch (bits) {
                case 32: {
                    return LLVMFloatType();
                }
                case 64: {
                    return LLVMDoubleType();
                }
                default: {
                    mabort(str("invalid bits"));
                }
            }
        }
        case type_struct: {
            Type_Struct* struct_ = &type->struct_;
            LLVMTypeRef* llvm_types = cap_alloc(struct_->field_count * sizeof(LLVMTypeRef));
            for (u64 i = 0; i < struct_->field_count; i++) {
                Type* type = struct_->field_types[i];
                LLVMTypeRef type_llvm = llvm_get_type(type);
                llvm_types[i] = type_llvm;
            }
            LLVMTypeRef struct_type = LLVMStructType(llvm_types, struct_->field_count, 0);
            return struct_type;
        }
        case type_multiple_value: {
            Type_Multiple_Value* multi_value = &type->multiple_value;
            LLVMTypeRef* llvm_types = cap_alloc(multi_value->types_count * sizeof(LLVMTypeRef));
            for (u64 i = 0; i < multi_value->types_count; i++) {
                Type* type = &multi_value->types[i];
                LLVMTypeRef type_llvm = llvm_get_type(type);
                llvm_types[i] = type_llvm;
            }
            LLVMTypeRef struct_type = LLVMStructType(llvm_types, multi_value->types_count, 0);
            return struct_type;
        }
        case type_void: {
            return LLVMVoidType();
        }
        case type_reference: {
            return LLVMPointerType(LLVMIntType(8), 0);
        }
        case type_pointer: {
            return LLVMPointerType(LLVMIntType(8), 0);
        }
        case type_invalid: {
            mabort(str("invalid type"));
            return NULL;
            break;
        }
    }
}

void _llvm_add_variable(Variable* variable, bool is_global) {
    LLVMTypeRef variable_type = llvm_get_type(&variable->type);
    String variable_name = variable->name;
    char variable_namec[4096];
    snprintf(variable_namec, 4096, "var$%.*s", str_info(variable_name));
    LLVMValueRef variable_value;
    if (is_global) {
        variable_value = LLVMAddGlobal(cap_context.llvm_info.module_info.module, variable_type, variable_namec);
        LLVMSetInitializer(variable_value, LLVMConstNull(variable_type));
        LLVMSetLinkage(variable_value, LLVMInternalLinkage);
    } else {
        variable_value = LLVMBuildAlloca(cap_context.llvm_info.builder, variable_type, variable_namec);
    }
    LLVMBuildStore(cap_context.llvm_info.builder, LLVMConstNull(variable_type), variable_value);
    llvm_set_variable_value(variable, variable_value);
}

void llvm_add_global_variable(Variable* variable) {
    return _llvm_add_variable(variable, true);
}

void llvm_add_variable(Variable* variable) {
    return _llvm_add_variable(variable, false);
}

LLVM_Scope_Info llvm_compile_scope(Scope* scope) {
    return llvm_compile_scope_with_function_variables(scope, NULL, 0);
}

LLVMBasicBlockRef llvm_add_global_statements() {
    Scope* last_scope = cap_context.scope;
    cap_context.scope = &cap_context.global_scope;

    Cap_Folder* folder = cap_context.folders[cap_context.namespace_we_are_in];
    Function_Implementation* function_implementation = cap_context.function_being_built;
    LLVM_Function_Implementation_Info* function_info = llvm_get_function_implementation_info(function_implementation);
    LLVMBasicBlockRef global_entry = LLVMAppendBasicBlock(function_info->function, "scope_entry");
    LLVMBasicBlockRef last_block = llvm_set_active_block(global_entry);
    LLVM_Scope_Info* scope_info = llvm_add_scope_info(&cap_context.global_scope);
    scope_info->entry_block = global_entry;

    Scope* scope = &cap_context.global_scope;
    for (u64 i = 0; i < scope->variables_count; i++) {
        Variable* variable = scope->variables[i];
        llvm_add_global_variable(variable);
    }

    for (u64 i = 0; i < folder->global_statements_count; i++) {
        Statement* global_statement = &folder->global_statements[i];
        bool breaks_scope = llvm_compile_statement(global_statement);
        massert(!breaks_scope, str("expected global statement to not break scope"));
    }

    cap_context.scope = last_scope;
    llvm_set_active_block(last_block);

    return global_entry;
}

LLVM_Scope_Info llvm_compile_scope_with_function_variables(Scope* scope, Variable** scope_variables_already_initalized,
                                                           u64 scope_variables_already_initalized_count) {
    Scope* last_scope = cap_context.scope;
    cap_context.scope = scope;
    Function_Implementation* function_implementation = cap_context.function_being_built;
    LLVM_Function_Implementation_Info* function_info = llvm_get_function_implementation_info(function_implementation);
    LLVMBasicBlockRef scope_entry_block = LLVMAppendBasicBlock(function_info->function, "scope_entry");
    LLVMBasicBlockRef last_block = llvm_set_active_block(scope_entry_block);
    LLVM_Scope_Info* scope_info = llvm_add_scope_info(scope);
    scope_info->entry_block = scope_entry_block;

    for (u64 i = 0; i < scope->variables_count; i++) {
        Variable* variable = scope->variables[i];
        i64 function_variable_index = -1;
        for (u64 j = 0; j < scope_variables_already_initalized_count; j++) {
            Variable* already_initalized_variable = scope_variables_already_initalized[j];
            if (variable == already_initalized_variable) function_variable_index = j;
        }
        llvm_add_variable(variable);
        if (function_variable_index >= 0) {
            LLVMValueRef variable_value = llvm_get_variable_value(variable);
            LLVMValueRef function_value = LLVMGetParam(function_info->function, function_variable_index);
            LLVMBuildStore(cap_context.llvm_info.builder, function_value, variable_value);
        }
    }

    LLVMBasicBlockRef scope_statements_block = LLVMAppendBasicBlock(function_info->function, "scope_statements");
    LLVMBuildBr(cap_context.llvm_info.builder, scope_statements_block);
    llvm_set_active_block(scope_statements_block);
    ptr_append(scope_info->statements_blocks, scope_info->statements_blocks_count, scope_info->statements_blocks_capacity, scope_statements_block);

    for (u64 i = 0; i < scope->statements_count; i++) {
        Statement* statement = &scope->statements[i];
        bool breaks_scope = llvm_compile_statement(statement);
        if (breaks_scope) break;
    }

    LLVM_Scope_Info info = *scope_info;
    llvm_pop_scope_info();
    cap_context.scope = last_scope;
    llvm_set_active_block(last_block);
    return info;
}

Type* __dereference_type__(Type* type) {
    Type t = sem_type_pointer(type, NULL);
    Type* type_ptr = cap_alloc(sizeof(Type));
    *type_ptr = t;
    return type_ptr;
}

LLVMValueRef llvm_compile_reference(Expression* expression) {
    massert(expression->kind == expression_reference, str("expected expression_reference"));
    Expression* expr = expression->dereference.expr;
    return llvm_compile_expression(expr);
}

LLVMValueRef llvm_compile_dereference(Expression* expression) {
    massert(expression->kind == expression_dereference, str("expected expression_dereference"));
    Expression* expr = expression->dereference.expr;
    LLVMValueRef value = llvm_compile_expression(expr);
    if (expr->type.kind == type_pointer) {
        return value;
    } else if (expr->type.kind == type_reference) {
        LLVMTypeRef type = llvm_get_type(&expression->type);
        LLVMValueRef deref_value = LLVMBuildLoad2(cap_context.llvm_info.builder, type, value, "$deref");
        return deref_value;
    } else if (expr->type.kind == type_type) {
        if (!cap_context.is_in_semantic_analysis) return llvm_zero_sized_value();
        Type int_type = sem_int_type(8);
        Type pointer_type = sem_type_pointer(&int_type, expression->ast);
        Type* pointer_type_ptr = cap_alloc(sizeof(Type));
        *pointer_type_ptr = pointer_type;
        Type** pointer_type_ptr_ptr = &pointer_type_ptr->pointer.underlying_type;

        LLVMValueRef type_to_override = llvm_get_pointer_as_llvm_value(pointer_type_ptr_ptr);
        LLVMBuildStore(cap_context.llvm_info.builder, value, type_to_override);

        LLVMValueRef new_pointer_ptr = llvm_get_pointer_as_llvm_value(pointer_type_ptr);
        return new_pointer_ptr;
    } else {
        mabort(str("trying to derefernce non dereferenceable type"));
    }
}

LLVMValueRef llvm_compile_cast_with_valueref(Expression* expression, LLVMValueRef value_of_cast) {
    massert(expression->kind == expression_cast, str("expected expression_cast"));
    Expression* underlying_expr = expression->cast.expr;
    LLVMValueRef underlying_value = value_of_cast;
    Type* underlying_type = &underlying_expr->type;
    Type* new_type = &expression->type;
    if (sem_type_equal(underlying_type, new_type, false, false)) return underlying_value;

    switch (new_type->kind) {
        case type_float_literal: {
            i64 new_bits = 64;
            switch (underlying_type->kind) {
                case type_int_literal: {
                    i64 old_bits = underlying_type->int_.bits;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildFPExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildFPTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_int: {
                    i64 old_bits = underlying_type->int_.bits;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildFPExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildFPTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_uint: {
                    i64 old_bits = underlying_type->uint.bits;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildFPExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildFPTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_float_literal: {
                    return underlying_value;
                }
                case type_float: {
                    i64 old_bits = underlying_type->float_.bits;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildFPExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildFPTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                default: {
                    mabort(str("invalid underlying type"));
                }
            }
        }
        case type_int_literal: {
            i64 new_bits = 64;
            switch (underlying_type->kind) {
                case type_int_literal: {
                    i64 old_bits = underlying_type->int_.bits;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildSExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_int: {
                    i64 old_bits = underlying_type->int_.bits;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildSExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_uint: {
                    i64 old_bits = underlying_type->uint.bits;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildZExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_float_literal: {
                    return LLVMBuildFPToSI(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                }
                case type_float: {
                    return LLVMBuildFPToSI(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                }
                default: {
                    mabort(str("invalid underlying type"));
                }
            }
        }
        case type_int: {
            i64 new_bits = new_type->int_.bits;
            switch (underlying_type->kind) {
                case type_int_literal: {
                    i64 old_bits = 64;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildSExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_int: {
                    i64 old_bits = underlying_type->int_.bits;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildSExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_uint: {
                    i64 old_bits = underlying_type->uint.bits;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildSExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_float_literal: {
                    return LLVMBuildFPToSI(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                }
                case type_float: {
                    return LLVMBuildFPToSI(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                }
                default: {
                    mabort(str("invalid underlying type"));
                }
            }
        }
        case type_uint: {
            i64 new_bits = new_type->uint.bits;
            switch (underlying_type->kind) {
                case type_int_literal: {
                    i64 old_bits = 64;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildZExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_int: {
                    i64 old_bits = underlying_type->int_.bits;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildZExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_uint: {
                    i64 old_bits = underlying_type->uint.bits;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildZExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_float_literal: {
                    return LLVMBuildFPToUI(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                }
                case type_float: {
                    return LLVMBuildFPToUI(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                }
                default: {
                    mabort(str("invalid underlying type"));
                }
            }
        }
        case type_float: {
            i64 new_bits = new_type->float_.bits;
            switch (underlying_type->kind) {
                case type_int_literal: {
                    return LLVMBuildSIToFP(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                }
                case type_int: {
                    return LLVMBuildSIToFP(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                }
                case type_uint: {
                    return LLVMBuildUIToFP(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                }
                case type_float_literal: {
                    i64 old_bits = 64;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildFPExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildFPTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                case type_float: {
                    i64 old_bits = underlying_type->float_.bits;
                    if (new_bits == old_bits) return underlying_value;
                    else if (new_bits > old_bits) {
                        return LLVMBuildFPExt(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    } else {
                        return LLVMBuildFPTrunc(cap_context.llvm_info.builder, underlying_value, llvm_get_type(new_type), "$cast");
                    }
                }
                default: {
                    mabort(str("invalid underlying type"));
                }
            }
        }
        case type_pointer: {
            return underlying_value;
        }
        case type_struct:
        case type_reference:
        case type_type:
        case type_void:
        case type_multiple_value:
        case type_invalid: {
            mabort(str("no known way to cast type"));
        }
    }
}

LLVMValueRef llvm_compile_cast(Expression* expression) {
    massert(expression->kind == expression_cast, str("expected expression_cast"));
    Expression* underlying_expr = expression->cast.expr;
    LLVMValueRef underlying_value = llvm_compile_expression(underlying_expr);
    return llvm_compile_cast_with_valueref(expression, underlying_value);
}

LLVM_Function_Implementation_Info* llvm_build_function_implementation(Function_Implementation* function_implementation) {
    Function_Implementation* last_function_being_built = cap_context.function_being_built;
    cap_context.function_being_built = function_implementation;
    LLVM_Function_Implementation_Info* function_info = llvm_add_function_implementation_info(function_implementation);

    // TODO: figure out if we need to pass the parameters allocator
    LLVMTypeRef* param_types = cap_alloc(function_implementation->parameter_count * sizeof(LLVMTypeRef));
    for (u64 i = 0; i < function_implementation->parameter_count; i++) {
        Type* parameter_type = &function_implementation->parameters[i]->type;
        LLVMTypeRef param_type = llvm_get_type(parameter_type);
        param_types[i] = param_type;
    }

    LLVMTypeRef return_type;
    if (function_implementation->return_types_count == 1) {
        return_type = llvm_get_type(&function_implementation->return_types[0]);
    } else {
        Type multiple_value = sem_type_multiple_value(function_implementation->return_types, function_implementation->return_types_count);
        return_type = llvm_get_type(&multiple_value);
    }

    LLVMTypeRef function_type = LLVMFunctionType(return_type, param_types, function_implementation->parameter_count, 0);
    char cName[2048];
    String function_name = function_implementation->function->name;
    snprintf(cName, 2048, "$%.*s", str_info(function_name));
    LLVMValueRef function_value = LLVMAddFunction(cap_context.llvm_info.module_info.module, cName, function_type);
    LLVMSetLinkage(function_value, LLVMInternalLinkage);
    function_info->function_type = function_type;
    function_info->function = function_value;

    LLVMBasicBlockRef function_entry_block = LLVMAppendBasicBlock(function_value, "$function_entry");
    LLVMBasicBlockRef last_block = llvm_set_active_block(function_entry_block);

    LLVM_Scope_Info info = llvm_compile_scope_with_function_variables(&function_implementation->body, function_implementation->parameters,
                                                                      function_implementation->parameter_count);
    LLVMPositionBuilderAtEnd(cap_context.llvm_info.builder, function_entry_block);
    LLVMBuildBr(cap_context.llvm_info.builder, info.entry_block);

    cap_context.function_being_built = last_function_being_built;
    llvm_set_active_block(last_block);

    return function_info;
}

LLVMValueRef llvm_compile_function_call_internal(Expression* expression) {
    LLVMValueRef* parameters = cap_alloc(expression->internal_call.parameter_count * sizeof(LLVMValueRef));
    for (u64 i = 0; i < expression->internal_call.parameter_count; i++) {
        Expression* parameter = &expression->internal_call.parameters[i];
        LLVMValueRef parameter_value = llvm_compile_expression(parameter);
        parameters[i] = parameter_value;
    }

    Function_Implementation* function_implementation = expression->internal_call.implementation;
    LLVM_Function_Implementation_Info* function_info = llvm_get_function_implementation_info(function_implementation);

    LLVMValueRef function_value = LLVMBuildCall2(cap_context.llvm_info.builder, function_info->function_type, function_info->function, parameters,
                                                 expression->internal_call.parameter_count, "$call_internal");

    // for function call origin tracking
    if (cap_context.is_in_semantic_analysis) {
        u64 offset_of_origin = offsetof(Type, origin);
        LLVMValueRef offset_of_origin_llvm = LLVMConstInt(LLVMInt64Type(), offset_of_origin, 0);

        u64 value_origin_count = 0;
        u64 value_origin_capacity = 0;
        LLVMValueRef* value_origin_ptrs = NULL;
        if (expression->type.kind == type_type) {
            LLVMValueRef value_origin_ptr =
                LLVMBuildGEP2(cap_context.llvm_info.builder, LLVMIntType(8), function_value, &offset_of_origin_llvm, 1, "$value_origin_ptr");
            ptr_append(value_origin_ptrs, value_origin_count, value_origin_capacity, value_origin_ptr);
        } else if (expression->type.kind == type_multiple_value) {
            Type_Multiple_Value* multiple_value = &expression->type.multiple_value;
            for (u64 i = 0; i < multiple_value->types_count; i++) {
                Type* type = &multiple_value->types[i];
                if (type->kind != type_type) continue;
                LLVMValueRef valuei_llvm = llvm_compile_multiple_values_access_with_valueref_and_index(function_value, i);
                LLVMValueRef value_origin_ptr =
                    LLVMBuildGEP2(cap_context.llvm_info.builder, LLVMIntType(8), valuei_llvm, &offset_of_origin_llvm, 1, "$value_origin_ptr");
                ptr_append(value_origin_ptrs, value_origin_count, value_origin_capacity, value_origin_ptr);
            }
        }
        if (value_origin_count != 0) {
            Type_Call_Origin* call_origin = cap_alloc(sizeof(Type_Call_Origin));
            call_origin->function = function_implementation->function;
            call_origin->parameter_types = cap_alloc(expression->internal_call.parameter_count * sizeof(Type));
            call_origin->parameter_compile_time_values = cap_alloc(expression->internal_call.parameter_count * sizeof(void*));
            call_origin->parameter_count = expression->internal_call.parameter_count;
            for (u64 i = 0; i < expression->internal_call.parameter_count; i++) {
                Expression* parameter = &expression->internal_call.parameters[i];
                LLVMValueRef parameter_value = parameters[i];
                Type* parameter_type = &parameter->type;
                call_origin->parameter_types[i] = *parameter_type;

                LLVMTypeRef parameter_type_llvm = llvm_get_type(parameter_type);
                u64 parameter_type_size = LLVMSizeOfTypeInBits(cap_context.llvm_info.data_layout, parameter_type_llvm);
                void* parameter_memory = cap_alloc(parameter_type_size);
                call_origin->parameter_compile_time_values[i] = parameter_memory;

                LLVMValueRef parameter_memory_llvm = llvm_get_pointer_as_llvm_value(parameter_memory);

                LLVMBuildStore(cap_context.llvm_info.builder, parameter_value, parameter_memory_llvm);
            }

            for (u64 i = 0; i < value_origin_count; i++) {
                Type_Origin* origin = cap_alloc(sizeof(Type_Origin));
                origin->kind = type_call_origin;
                origin->previous = cap_alloc(sizeof(Type_Origin));
                origin->previous->kind = 990;
                origin->call_origin = call_origin;

                u64 origin_size = sizeof(Type_Origin);
                LLVMValueRef origin_size_llvm = LLVMConstInt(LLVMInt64Type(), origin_size, 0);
                LLVMValueRef origin_ptr_llvm = value_origin_ptrs[i];

                LLVMValueRef previous_ptr_llvm = llvm_get_pointer_as_llvm_value(origin->previous);
                LLVMBuildMemCpy(cap_context.llvm_info.builder, previous_ptr_llvm, 0, origin_ptr_llvm, 0, origin_size_llvm);

                LLVMValueRef origin_ptr = llvm_get_pointer_as_llvm_value(origin);
                LLVMBuildMemCpy(cap_context.llvm_info.builder, origin_ptr_llvm, 0, origin_ptr, 0, origin_size_llvm);
            }
        }
    }

    return function_value;
}

LLVMValueRef llvm_compile_int(Expression* expression) {
    i64 value = expression->int_value.value;
    return LLVMConstInt(LLVMIntType(64), value, false);
}

LLVMValueRef llvm_compile_float(Expression* expression) {
    f64 value = expression->float_value.value;
    return LLVMConstReal(LLVMDoubleType(), value);
}

LLVMValueRef llvm_extract_multiple_values_value(LLVMValueRef value, u64 index, Type* type) {
    return LLVMBuildExtractValue(cap_context.llvm_info.builder, value, index, "$");
}

LLVMValueRef llvm_compile_multiple_values_access_with_valueref_and_index(LLVMValueRef value_of_multiple_value, u64 index) {
    return LLVMBuildExtractValue(cap_context.llvm_info.builder, value_of_multiple_value, index, "$");
}

LLVMValueRef llvm_compile_multiple_values_access_with_valueref(Expression* expression, LLVMValueRef value_of_multiple_value) {
    massert(expression->kind == expression_multiple_values_access, str("expected multiple values"));
    Expression* multiple_values_value = expression->multiple_values_access.multiple_values_value;
    Type* multiple_values_type = &multiple_values_value->type;
    return llvm_extract_multiple_values_value(value_of_multiple_value, expression->multiple_values_access.index, multiple_values_type);
}

LLVMValueRef llvm_compile_multiple_values_access(Expression* expression) {
    Expression* multiple_values_value = expression->multiple_values_access.multiple_values_value;
    LLVMValueRef value = llvm_compile_expression(multiple_values_value);
    return llvm_compile_multiple_values_access_with_valueref(expression, value);
}

LLVMValueRef llvm_compile_struct(Expression* expression) {
    if (!cap_context.is_in_semantic_analysis) return llvm_zero_sized_value();
    Type* new_struct_type = cap_alloc(sizeof(Type));
    new_struct_type->kind = type_struct;
    new_struct_type->allocator_id = sem_get_new_allocator_id();
    new_struct_type->struct_.field_names = cap_alloc(expression->struct_.field_count * sizeof(String));
    new_struct_type->struct_.field_types = cap_alloc(expression->struct_.field_count * sizeof(Type));
    new_struct_type->struct_.field_count = expression->struct_.field_count;
    new_struct_type->origin.previous = NULL;
    new_struct_type->origin.kind = type_intrisic_origin;

    LLVMValueRef* field_type_values = cap_alloc(expression->struct_.field_count * sizeof(LLVMValueRef));
    for (u64 i = 0; i < expression->struct_.field_count; i++) {
        Expression* field_type = &expression->struct_.field_types[i];
        LLVMValueRef field_type_value = llvm_compile_expression(field_type);
        field_type_values[i] = field_type_value;
        new_struct_type->struct_.field_names[i] = expression->struct_.field_names[i];
        void* value_to_read_type_into = &new_struct_type->struct_.field_types[i];

        LLVMValueRef llvm_ptr = llvm_get_pointer_as_llvm_value(value_to_read_type_into);

        LLVMBuildStore(cap_context.llvm_info.builder, field_type_value, llvm_ptr);
    }

    LLVMValueRef llvm_ptr = llvm_get_pointer_as_llvm_value(new_struct_type);

    return llvm_ptr;
}

LLVMValueRef llvm_compile_struct_field_access(Expression* expression) {
    massert(expression->kind == expression_struct_field_access, str("expected expression_struct_field_access"));
    Expression* lhs = expression->struct_field_access.struct_value;
    LLVMValueRef value = llvm_compile_expression(lhs);

    Type* lhs_type = &lhs->type;
    massert(lhs_type->kind == type_struct || lhs_type->kind == type_reference, str("expected struct or reference"));
    Type struct_type;
    if (lhs_type->kind == type_reference) {
        struct_type = sem_type_dereference(lhs_type);
    } else {
        struct_type = *lhs_type;
    }

    LLVMTypeRef struct_type_llvm = llvm_get_type(&struct_type);
    if (lhs_type->kind == type_reference) {
        return LLVMBuildStructGEP2(cap_context.llvm_info.builder, struct_type_llvm, value, expression->struct_field_access.field_index, "$struct_field_access");
    } else {
        return LLVMBuildExtractValue(cap_context.llvm_info.builder, value, expression->struct_field_access.field_index, "$struct_field_access");
    }
}

LLVMValueRef llvm_zero_sized_value() {
    LLVMTypeRef zero_sized_struct = LLVMStructType(NULL, 0, 0);
    return LLVMGetUndef(zero_sized_struct);
}

LLVMValueRef llvm_compile_alloc(Expression* expression) {
    massert(expression->kind == expression_alloc, str("expected expression_alloc"));

    Type* type_to_allocate = &expression->alloc.type_to_allocate;
    LLVMTypeRef type_to_allocate_llvm = llvm_get_type(type_to_allocate);
    Expression* count = expression->alloc.count;
    LLVMValueRef count_value = llvm_compile_expression(count);

    Type* type_ptr = &expression->type;
    u64 allocator_id = type_ptr->allocator_id;
    Allocator* allocator = sem_get_allocator(allocator_id);

    if (allocator->variable == NULL) {
    }

    massert(false, str("not implemented"));
    return NULL;
}

LLVMValueRef llvm_compile_function_call_external(Expression* expression) {
    massert(expression->kind == expression_function_call_external, str("expected expression_function_call_external"));

    LLVMValueRef* parameters = cap_alloc(expression->external_call.parameter_count * sizeof(LLVMValueRef));
    for (u64 i = 0; i < expression->external_call.parameter_count; i++) {
        Expression* parameter = &expression->external_call.parameters[i];
        LLVMValueRef parameter_value = llvm_compile_expression(parameter);
        parameters[i] = parameter_value;
    }

    Function* function = expression->external_call.function;
    LLVM_Function_Info* function_info = llvm_get_function_info(function);

    LLVMValueRef function_value = LLVMBuildCall2(cap_context.llvm_info.builder, function_info->function_type, function_info->function, parameters,
                                                 expression->external_call.parameter_count, "$call_external");
    return function_value;
}

LLVMValueRef llvm_compile_compile_time_value(Expression* expression) {
    void* value = expression->compile_time_value.value;
    LLVMValueRef llvm_ptr = llvm_get_pointer_as_llvm_value(value);
    Type* type = &expression->type;
    LLVMTypeRef llvm_type = llvm_get_type(type);
    return LLVMBuildLoad2(cap_context.llvm_info.builder, llvm_type, llvm_ptr, "$compile_time_value");
}

LLVMValueRef llvm_compile_int_type(Expression* expression) {
    if (!cap_context.is_in_semantic_analysis) return llvm_zero_sized_value();
    i64 bits = expression->int_type.bits;
    Type type = sem_int_type(bits);
    Type* type_ptr = cap_alloc(sizeof(Type));
    *type_ptr = type;
    LLVMValueRef llvm_ptr = llvm_get_pointer_as_llvm_value(type_ptr);
    return llvm_ptr;
}

LLVMValueRef llvm_compile_uint_type(Expression* expression) {
    if (!cap_context.is_in_semantic_analysis) return llvm_zero_sized_value();
    i64 bits = expression->int_type.bits;
    Type type = sem_uint_type(bits);
    Type* type_ptr = cap_alloc(sizeof(Type));
    *type_ptr = type;
    LLVMValueRef llvm_ptr = llvm_get_pointer_as_llvm_value(type_ptr);
    return llvm_ptr;
}

LLVMValueRef llvm_compile_expression(Expression* expression) {
    switch (expression->kind) {
        case expression_uint_type:
            return llvm_compile_uint_type(expression);
        case expression_int_type:
            return llvm_compile_int_type(expression);
        case expression_compile_time_value:
            return llvm_compile_compile_time_value(expression);
        case expression_alloc:
            return llvm_compile_alloc(expression);
        case expression_struct_field_access:
            return llvm_compile_struct_field_access(expression);
        case expression_passthrough:
            return llvm_compile_expression(expression->passthrough.expr);
        case expression_variable:
            return llvm_get_variable_value(expression->variable.variable);
        case expression_variable_declaration:
            return llvm_get_variable_value(expression->variable_declaration.variable);
        case expression_multiple_values_access:
            return llvm_compile_multiple_values_access(expression);
        case expression_dereference:
            return llvm_compile_dereference(expression);
        case expression_cast:
            return llvm_compile_cast(expression);
        case expression_reference:
            return llvm_compile_reference(expression);
        case expression_function_call_external:
            return llvm_compile_function_call_external(expression);
        case expression_function_call_internal:
            return llvm_compile_function_call_internal(expression);
        case expression_int:
            return llvm_compile_int(expression);
        case expression_float:
            return llvm_compile_float(expression);
        case expression_struct:
            return llvm_compile_struct(expression);
        case expression_invalid: {
            mabort(str("invalid expression"));
        }
    }
}

bool llvm_compile_expression_statement(Statement* statement) {
    massert(statement->kind == statement_expression, str("expected statement_expression"));
    Expression* expression = &statement->expression.expression;
    llvm_compile_expression(expression);
    return false;
}

bool llvm_compile_assignment_multiple_values_statement(Statement* statement) {
    massert(statement->kind == statement_assignment_multiple_values, str("expected statement_assignment_multiple_values"));
    Statement_Assignment_Multiple_Values* assignment = &statement->assignment_multiple_values;
    LLVMValueRef multiple_values_value = llvm_compile_expression(assignment->multiple_values_value);

    LLVMValueRef* assignee_values = cap_alloc(assignment->count * sizeof(LLVMValueRef));
    for (u64 i = 0; i < assignment->count; i++) {
        Expression* assignee = &assignment->assignees[i];
        LLVMValueRef assignee_value = llvm_compile_expression(assignee);
        assignee_values[i] = assignee_value;
    }

    LLVMValueRef* values = cap_alloc(assignment->count * sizeof(LLVMValueRef));
    for (u64 i = 0; i < assignment->count; i++) {
        Expression* value = &assignment->values[i];
        if (value->kind == expression_cast) {
            Expression* underlying_expr = value->cast.expr;
            LLVMValueRef underlying_value;
            if (underlying_expr->kind == expression_multiple_values_access) {
                underlying_value = llvm_compile_multiple_values_access_with_valueref(underlying_expr, multiple_values_value);
            } else {
                mabort(str("expected multiple values access"));
            }
            values[i] = llvm_compile_cast_with_valueref(value, underlying_value);
        } else if (value->kind == expression_multiple_values_access) {
            values[i] = llvm_compile_multiple_values_access_with_valueref(value, multiple_values_value);
        } else {
            mabort(str("expected cast or multiple values access"));
        }
    }

    for (u64 i = 0; i < assignment->count; i++) {
        LLVMValueRef assignee_value = assignee_values[i];
        LLVMValueRef value_value = values[i];
        Expression* value = &assignment->values[i];
        Expression* assignee = &assignment->assignees[i];
        Type* assignee_type = &assignee->type;
        Type* value_type = &value->type;
        massert(assignee_type->kind == type_reference, str("expected pointer"));
        LLVMTypeRef value_type_llvm = llvm_get_type(value_type);
        LLVMBuildStore(cap_context.llvm_info.builder, value_value, assignee_value);
    }
    return false;
}

bool llvm_compile_assignment_statement(Statement* statement) {
    massert(statement->kind == statement_assignment, str("expected statement_assignment"));
    Statement_Assignment* assignment = &statement->assignment;
    massert(assignment->assignees_count >= assignment->values_count, str("expected assignees_count to be greater than or equal to values_count"));
    LLVMValueRef* assignee_values = cap_alloc(assignment->assignees_count * sizeof(LLVMValueRef));
    for (u64 i = 0; i < assignment->assignees_count; i++) {
        Expression* assignee = &assignment->assignees[i];
        LLVMValueRef assignee_value = llvm_compile_expression(assignee);
        assignee_values[i] = assignee_value;
    }
    for (u64 i = 0; i < assignment->values_count; i++) {
        Expression* value = &assignment->values[i];
        LLVMValueRef value_value = llvm_compile_expression(value);

        LLVMValueRef assignee_value = assignee_values[i];
        Expression* assignee = &assignment->assignees[i];
        Type* assignee_type = &assignee->type;
        massert(assignee_type->kind == type_reference, str("expected pointer"));
        Type* value_type = &value->type;
        LLVMTypeRef value_type_llvm = llvm_get_type(value_type);
        LLVMBuildStore(cap_context.llvm_info.builder, value_value, assignee_value);
    }

    return false;
}

bool llvm_compile_return_statement(Statement* statement) {
    massert(statement->kind == statement_return, str("expected statement_return"));
    if (statement->return_.values_count == 0) {
        LLVMBuildRetVoid(cap_context.llvm_info.builder);
        return true;
    }

    LLVMValueRef* values = cap_alloc(statement->return_.values_count * sizeof(LLVMValueRef));
    for (u64 i = 0; i < statement->return_.values_count; i++) {
        Expression* expression = &statement->return_.values[i];
        LLVMValueRef value = llvm_compile_expression(expression);
        values[i] = value;
    }

    LLVMValueRef return_value;
    if (statement->return_.values_count == 1) {
        return_value = values[0];
    } else {
        Function_Implementation* function_implementation = cap_context.function_being_built;
        Type multiple_type = sem_type_multiple_value(function_implementation->return_types, function_implementation->return_types_count);
        LLVMTypeRef multiple_type_llvm = llvm_get_type(&multiple_type);
        LLVMValueRef struct_value = LLVMGetUndef(multiple_type_llvm);
        for (u64 i = 0; i < statement->return_.values_count; i++) {
            LLVMValueRef value = values[i];
            struct_value = LLVMBuildInsertValue(cap_context.llvm_info.builder, struct_value, value, i, "$");
        }
        return_value = struct_value;
    }

    LLVMBuildRet(cap_context.llvm_info.builder, return_value);
    return true;
}

bool llvm_compile_statement(Statement* statement) {
    switch (statement->kind) {
        case statement_expression: {
            return llvm_compile_expression_statement(statement);
        }
        case statement_assignment_multiple_values: {
            return llvm_compile_assignment_multiple_values_statement(statement);
        }
        case statement_assignment: {
            return llvm_compile_assignment_statement(statement);
        }
        case statement_return: {
            return llvm_compile_return_statement(statement);
        }
        case statement_function_declaration: {
            return false;
        }
        case statement_invalid: {
            massert(false, str("should never be compiling an invalid statement"));
            return false;
        }
    }
}

void llvm_compile_program(Program* program) {
    LLVM_Module_Info last_module = llvm_create_new_module();
    Function_Implementation* last_function_being_built = cap_context.function_being_built;

    Function* main_function = &program->function;
    Function_Implementation* main_function_implementation = main_function->internal.implementations[0];
    Scope* main_function_scope = &main_function_implementation->body;

    cap_context.function_being_built = main_function_implementation;

    LLVMTypeRef main_function_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    LLVMValueRef main_function_value = LLVMAddFunction(cap_context.llvm_info.module_info.module, "main", main_function_type);
    LLVM_Function_Implementation_Info* function_info = llvm_add_function_implementation_info(main_function_implementation);
    function_info->function = main_function_value;
    function_info->function_type = main_function_type;

    LLVMBasicBlockRef entry = llvm_add_global_statements();

    LLVM_Scope_Info scope_info = llvm_compile_scope(main_function_scope);
    LLVMBasicBlockRef scope_entry_block = scope_info.entry_block;

    llvm_set_active_block(entry);
    LLVMBuildBr(cap_context.llvm_info.builder, scope_entry_block);

    llvm_print_module();

    char* error;
    if (LLVMVerifyModule(cap_context.llvm_info.module_info.module, LLVMAbortProcessAction, &error) != 0) {
        String error_str = string_create(error, strlen(error));
        String error_message = string_append(str("Failed to verify module: "), error_str);
        mabort(error_message);
    }

    String object_file_name = string_append(program->name, str(".obj"));
    String object_file_path = string_append(cap_context.build_directory, object_file_name);
    char object_file_pathc[4096];
    snprintf(object_file_pathc, 4096, "%.*s", str_info(object_file_path));
    if (LLVMTargetMachineEmitToFile(cap_context.llvm_info.target_machine, cap_context.llvm_info.module_info.module, object_file_pathc, LLVMObjectFile,
                                    &error) != 0) {
        String error_str = string_create(error, strlen(error));
        String error_message = string_append(str("Failed to emit object file: "), error_str);
        mabort(error_message);
    }

    String exe_file_name = string_append(program->name, str(".exe"));
    String exe_file_path = string_append(cap_context.build_directory, exe_file_name);
    if (!llvm_link_executable(exe_file_path, &object_file_path, 1)) {
        mabort(str("Failed to link executable"));
    };

    // run executable
    char commandc[4096];
    snprintf(commandc, 4096, "%.*s", str_info(exe_file_path));
    printf("command: %s\n", commandc);
    int result = system(commandc);
    rainbow_printf("\nresult: %d\n", result);

    LLVMDisposeModule(cap_context.llvm_info.module_info.module);
    cap_context.llvm_info.module_info = last_module;
    cap_context.function_being_built = last_function_being_built;
}

// last module
LLVM_Module_Info llvm_create_new_module() {
    LLVM_Module_Info last_module_info = cap_context.llvm_info.module_info;
    static u64 module_number = 0;
    char cName[2048];
    snprintf(cName, 2048, "__compiler_module__%llu", module_number);
    LLVMModuleRef program_module = LLVMModuleCreateWithName(cName);
    cap_context.llvm_info.module_info.module = program_module;
    return last_module_info;
}

void* llvm_evaluate_expression(Expression* expression) {
    LLVM_Module_Info last_module = llvm_create_new_module();

    static u64 function_number = 0;
    char cFunctionName[2048];
    snprintf(cFunctionName, 2048, "__expr_function__%llu", function_number);
    function_number += 1;
    LLVMTypeRef main_function_type = LLVMFunctionType(LLVMVoidType(), NULL, 0, 0);
    LLVMValueRef main_function_value = LLVMAddFunction(cap_context.llvm_info.module_info.module, cFunctionName, main_function_type);

    LLVM_Function_Implementation_Info* info = llvm_add_function_implementation_info(NULL);
    info->function = main_function_value;
    info->function_type = main_function_type;
    Function_Implementation* last_function_being_built = cap_context.function_being_built;
    cap_context.function_being_built = NULL;

    LLVMBasicBlockRef main_entry_block = LLVMAppendBasicBlock(main_function_value, "program_entry");
    llvm_set_active_block(main_entry_block);

    LLVMValueRef expression_value = llvm_compile_expression(expression);

    LLVMTypeRef expression_type = llvm_get_type(&expression->type);
    u64 expression_type_size = LLVMSizeOfTypeInBits(cap_context.llvm_info.data_layout, expression_type);
    void* return_memory = cap_alloc(expression_type_size);

    LLVMValueRef memory = LLVMConstInt(LLVMInt64Type(), (uintptr_t)return_memory, 0);
    memory = LLVMConstIntToPtr(memory, LLVMPointerType(expression_type, 0));

    LLVMBuildStore(cap_context.llvm_info.builder, expression_value, memory);
    LLVMBuildRetVoid(cap_context.llvm_info.builder);

    llvm_print_module();

    LLVMExecutionEngineRef engine = {0};
    char* error = NULL;
    if (LLVMCreateInterpreterForModule(&engine, cap_context.llvm_info.module_info.module, &error) != 0) {
        fprintf(stderr, "Failed to create interpreter: %s\n", error);
        LLVMDisposeMessage(error);
        mabort(str("Failed to create interpreter"));
    }
    LLVMGenericValueRef result = LLVMRunFunction(engine, main_function_value, 0, NULL);

    cap_context.llvm_info.module_info = last_module;
    cap_context.function_being_built = last_function_being_built;

    LLVMDisposeGenericValue(result);
    LLVMDisposeExecutionEngine(engine);
    return return_memory;
}

bool llvm_link_executable(String exe_file_path, String* object_file_paths, u64 count) {
    massert(count > 0, str("count must be greater than 0"));
    String object_file_paths_joined = object_file_paths[0];
    for (u64 i = 1; i < count; i++) {
        String object_file_path = object_file_paths[i];
        object_file_paths_joined = string_append(object_file_paths_joined, str(" "));
        object_file_paths_joined = string_append(object_file_paths_joined, object_file_path);
    }
    char link_command[4096];
    snprintf(link_command, 4096, "lld-link /OUT:%.*s /SUBSYSTEM:CONSOLE %.*s /DEFAULTLIB:libcmt", str_info(exe_file_path), str_info(object_file_paths_joined));
    return system(link_command) == 0;
}

LLVMBasicBlockRef llvm_set_active_block(LLVMBasicBlockRef block) {
    LLVMBasicBlockRef last_block = cap_context.llvm_info.active_block;
    LLVMPositionBuilderAtEnd(cap_context.llvm_info.builder, block);
    cap_context.llvm_info.active_block = block;
    return last_block;
}

void llvm_set_variable_value(Variable* variable, LLVMValueRef value) {
    Scope* scope = cap_context.scope;
    LLVM_Scope_Info* scope_info = llvm_get_scope_info(scope);
    LLVMValue_Variable_Pair pair = {value, variable};
    ptr_append(scope_info->variable_to_values, scope_info->variable_to_values_count, scope_info->variable_to_values_capacity, pair);
}

LLVMValueRef _llvm_get_variable_value(Variable* variable, Scope* scope) {
    LLVM_Scope_Info* scope_info = llvm_get_scope_info(scope);
    for (i64 i = (i64)scope_info->variable_to_values_count - 1; i >= 0; i--) {
        LLVMValue_Variable_Pair pair = scope_info->variable_to_values[i];
        if (pair.variable == variable) return pair.value;
    }
    if (scope->parent != NULL) return _llvm_get_variable_value(variable, scope->parent);
    mabort(str("variable not found"));
    return NULL;
}

LLVMValueRef llvm_get_pointer_as_llvm_value(void* value) {
    LLVMValueRef ptr_as_int = LLVMConstInt(LLVMIntType(64), (uintptr_t)value, 0);
    LLVMValueRef llvm_ptr = LLVMConstIntToPtr(ptr_as_int, LLVMPointerType(LLVMIntType(8), 0));
    return llvm_ptr;
}

LLVMValueRef llvm_get_variable_value(Variable* variable) {
    if (cap_context.is_in_semantic_analysis && variable->compile_time_value) {
        void* value = variable->compile_time_value;
        Type* type = value;
        return llvm_get_pointer_as_llvm_value(value);
    }
    Scope* scope = cap_context.scope;
    return _llvm_get_variable_value(variable, scope);
}

LLVM_Scope_Info* llvm_get_scope_info(Scope* scope) {
    for (i64 i = cap_context.llvm_info.module_info.scope_infos_count - 1; i >= 0; i--) {
        LLVM_Scope_Info_Scope_Pair* pair = &cap_context.llvm_info.module_info.scope_infos[i];
        if (pair->scope == scope) return &pair->scope_info;
    }
    mabort(str("scope not found"));
    return NULL;
}

LLVM_Scope_Info* llvm_add_scope_info(Scope* scope) {
    LLVM_Scope_Info scope_info = {0};
    LLVM_Scope_Info_Scope_Pair pair = {scope, scope_info};
    ptr_append(cap_context.llvm_info.module_info.scope_infos, cap_context.llvm_info.module_info.scope_infos_count,
               cap_context.llvm_info.module_info.scope_infos_capacity, pair);
    return &cap_context.llvm_info.module_info.scope_infos[cap_context.llvm_info.module_info.scope_infos_count - 1].scope_info;
}

void llvm_pop_scope_info() {
    cap_context.llvm_info.module_info.scope_infos_count--;
}

LLVM_Function_Info* llvm_add_function_info(Function* function) {
    LLVM_Function_Info function_info = {0};
    LLVM_Function_Info_Function_Pair pair = {function, function_info};
    ptr_append(cap_context.llvm_info.module_info.function_infos_by_name, cap_context.llvm_info.module_info.function_infos_by_name_count,
               cap_context.llvm_info.module_info.function_infos_by_name_capacity, pair);
    return &cap_context.llvm_info.module_info.function_infos_by_name[cap_context.llvm_info.module_info.function_infos_by_name_count - 1].function_info;
}
LLVM_Function_Info* llvm_get_function_info(Function* function) {
    for (i64 i = cap_context.llvm_info.module_info.function_infos_by_name_count - 1; i >= 0; i--) {
        LLVM_Function_Info_Function_Pair* pair = &cap_context.llvm_info.module_info.function_infos_by_name[i];
        if (pair->function == function) return &pair->function_info;
    }
    return llvm_build_function(function);
}
LLVM_Function_Info* llvm_build_function(Function* function) {
    massert(function->kind == function_external, str("expected external function"));
    LLVMTypeRef* param_types = cap_alloc(function->external.parameter_count * sizeof(LLVMTypeRef));
    for (u64 i = 0; i < function->external.parameter_count; i++) {
        Type* parameter_type = &function->external.parameter_types[i];
        LLVMTypeRef param_type = llvm_get_type(parameter_type);
        param_types[i] = param_type;
    }

    Type* return_type = &function->external.return_type;

    LLVMTypeRef return_type_llvm = llvm_get_type(return_type);

    String function_name = function->name;

    LLVMTypeRef function_type = LLVMFunctionType(return_type_llvm, param_types, function->external.parameter_count, 0);
    static u64 function_number = 0;
    char cName[2048];
    snprintf(cName, 2048, "%.*s", str_info(function_name));
    LLVMValueRef function_value = LLVMAddFunction(cap_context.llvm_info.module_info.module, cName, function_type);
    const char* actual_name = LLVMGetValueName(function_value);
    if (strcmp(actual_name, cName) != 0) {
        log_warning("Function '%s' was renamed to '%s' by LLVM (name collision)", cName, actual_name);
    }
    LLVMSetLinkage(function_value, LLVMExternalLinkage);
    LLVM_Function_Info function_info = {0};
    function_info.function = function_value;
    function_info.function_type = function_type;

    LLVM_Function_Info* info = llvm_add_function_info(function);
    info->function = function_value;
    info->function_type = function_type;
    return info;
}

LLVM_Function_Implementation_Info* llvm_add_function_implementation_info(Function_Implementation* function_implementation) {
    LLVM_Function_Implementation_Info function_info = {0};
    LLVM_Function_Info_Function_Implementation_Pair pair = {function_implementation, function_info};
    ptr_append(cap_context.llvm_info.module_info.function_infos, cap_context.llvm_info.module_info.function_infos_count,
               cap_context.llvm_info.module_info.function_infos_capacity, pair);
    return &cap_context.llvm_info.module_info.function_infos[cap_context.llvm_info.module_info.function_infos_count - 1].function_info;
}

LLVM_Function_Implementation_Info* llvm_get_function_implementation_info(Function_Implementation* function_implementation) {
    for (i64 i = cap_context.llvm_info.module_info.function_infos_count - 1; i >= 0; i--) {
        LLVM_Function_Info_Function_Implementation_Pair* pair = &cap_context.llvm_info.module_info.function_infos[i];
        if (pair->function_implementation == function_implementation) return &pair->function_info;
    }
    return llvm_build_function_implementation(function_implementation);
}
