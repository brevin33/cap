#include <llvm-c/Core.h>

#include "cap.h"
#include "cap/semantic.h"

LLVMTypeRef llvm_get_type(Type* type) {
    if (type->ptr_count > 0) {
        type->ptr_count -= 1;
        LLVMTypeRef ptr_type = llvm_get_type(type);
        type->ptr_count += 1;
        return LLVMPointerType(ptr_type, type->ptr_count);
    }
    if (type->is_ref) {
        type->is_ref = false;
        LLVMTypeRef ref_type = llvm_get_type(type);
        type->is_ref = true;
        return LLVMPointerType(ref_type, 0);
    }
    switch (type->base->kind) {
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

LLVMValueRef llvm_variable_to_llvm(LLVM_Scope* scope, Variable* variable) {
    for (u32 i = 0; i < scope->variables.count; i++) {
        LLVM_Variable_Pair* pair = LLVM_Variable_Pair_List_get(&scope->variables, i);
        Variable* pair_variable = pair->variable;
        if (pair_variable == variable) {
            return pair->value;
        }
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

LLVM_Scope llvm_compile_scope(Scope* scope, LLVM_Function* function) {
    LLVM_Scope llvm_scope = {0};
    llvm_scope.scope = scope;

    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(function->function_value, "entry");
    llvm_scope.block = entry_block;
    LLVMPositionBuilderAtEnd(cap_context.llvm_info.builder, entry_block);

    for (u32 i = 0; i < scope->variables.count; i++) {
        Variable* variable = *Variable_Ptr_List_get(&scope->variables, i);
        Allocator* allocator = sem_allocator_get(function->templated_function, variable->type.ref_allocator_id);
        if (sem_allocator_are_the_same(allocator, &STACK_ALLOCATOR)) {
            LLVMTypeRef llvm_type = llvm_get_type(&variable->type);
            LLVMValueRef alloca = LLVMBuildAlloca(cap_context.llvm_info.builder, llvm_type, variable->name);
            LLVMValueRef zero = LLVMConstNull(llvm_type);
            LLVMBuildStore(cap_context.llvm_info.builder, zero, alloca);
            llvm_store_variable_llvm_value(&llvm_scope, variable, alloca);
        }
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

LLVMValueRef llvm_build_expression(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    switch (expression->kind) {
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
        case expression_invalid: {
            massert(false, "should never happen");
            break;
        }
    }
}

LLVMValueRef llvm_build_expression_cast(Expression* expression, LLVM_Scope* scope, LLVM_Function* function) {
    Expression* from_expression = expression->cast.expression;
    Type from_type = from_expression->type;
    Type to_type = expression->type;
    LLVMValueRef from_value = llvm_build_expression(from_expression, scope, function);

    if (sem_type_is_equal_without_allocator_id(&from_type, &to_type)) {
        return from_value;
    }

    if (from_type.is_ref && !to_type.is_ref) {
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        LLVMValueRef deref_value = LLVMBuildLoad2(cap_context.llvm_info.builder, to_type_llvm_type, from_value, "");
        return deref_value;
    }

    if (from_type.base->kind == type_int && to_type.base->kind == type_int) {
        u64 from_bit_size = from_type.base->number_bit_size;
        u64 to_bit_size = to_type.base->number_bit_size;
        if (from_bit_size < to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildSExt(cap_context.llvm_info.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        } else if (from_bit_size > to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildTrunc(cap_context.llvm_info.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        }
        return from_value;
    }

    if (from_type.base->kind == type_uint && to_type.base->kind == type_uint) {
        u64 from_bit_size = from_type.base->number_bit_size;
        u64 to_bit_size = to_type.base->number_bit_size;
        if (from_bit_size < to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildZExt(cap_context.llvm_info.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        } else if (from_bit_size > to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildTrunc(cap_context.llvm_info.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        }
        return from_value;
    }

    if (from_type.base->kind == type_float && to_type.base->kind == type_float) {
        LLVMTypeRef from_llvm_type = llvm_get_type(&from_type);
        LLVMTypeRef to_llvm_type = llvm_get_type(&to_type);
        u64 from_bit_size = from_type.base->number_bit_size;
        u64 to_bit_size = to_type.base->number_bit_size;

        if (from_bit_size < to_bit_size) {
            return LLVMBuildFPExt(cap_context.llvm_info.builder, from_value, to_llvm_type, "");
        } else if (from_bit_size > to_bit_size) {
            return LLVMBuildFPTrunc(cap_context.llvm_info.builder, from_value, to_llvm_type, "");
        }
        return from_value;
    }

    if (from_type.base->kind == type_int && to_type.base->kind == type_uint) {
        u64 from_bit_size = from_type.base->number_bit_size;
        u64 to_bit_size = to_type.base->number_bit_size;
        if (from_bit_size < to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildZExt(cap_context.llvm_info.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        } else if (from_bit_size > to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildTrunc(cap_context.llvm_info.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        }
        return from_value;
    }

    if (from_type.base->kind == type_uint && to_type.base->kind == type_int) {
        u64 from_bit_size = from_type.base->number_bit_size;
        u64 to_bit_size = to_type.base->number_bit_size;
        if (from_bit_size < to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildZExt(cap_context.llvm_info.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        } else if (from_bit_size > to_bit_size) {
            LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
            LLVMValueRef cast_value = LLVMBuildTrunc(cap_context.llvm_info.builder, from_value, to_type_llvm_type, "");
            return cast_value;
        }
        return from_value;
    }

    if (from_type.base->kind == type_int && to_type.base->kind == type_float) {
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        LLVMValueRef cast_value = LLVMBuildSIToFP(cap_context.llvm_info.builder, from_value, to_type_llvm_type, "");
        return cast_value;
    }

    if (from_type.base->kind == type_uint && to_type.base->kind == type_float) {
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        LLVMValueRef cast_value = LLVMBuildUIToFP(cap_context.llvm_info.builder, from_value, to_type_llvm_type, "");
        return cast_value;
    }

    if (from_type.base->kind == type_const_int && to_type.base->kind == type_int) {
        char* value = llvm_evaluate_const_int(from_expression);
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        return LLVMConstIntOfString(to_type_llvm_type, value, 10);
    }

    if (from_type.base->kind == type_const_int && to_type.base->kind == type_uint) {
        char* value = llvm_evaluate_const_int(from_expression);
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        return LLVMConstIntOfString(to_type_llvm_type, value, 10);
    }

    if (from_type.base->kind == type_const_int && to_type.base->kind == type_float) {
        char* value = llvm_evaluate_const_int(from_expression);
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        return LLVMConstRealOfString(to_type_llvm_type, value);
    }

    if (from_type.base->kind == type_const_float && to_type.base->kind == type_float) {
        double value = llvm_evaluate_const_float(from_expression);
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        return LLVMConstReal(to_type_llvm_type, value);
    }

    if (from_type.base->kind == type_const_float && to_type.base->kind == type_int) {
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        double value = llvm_evaluate_const_float(from_expression);
        i64 value_i64 = (i64)value;
        return LLVMConstInt(to_type_llvm_type, value_i64, true);
    }

    if (from_type.base->kind == type_const_float && to_type.base->kind == type_uint) {
        LLVMTypeRef to_type_llvm_type = llvm_get_type(&to_type);
        double value = llvm_evaluate_const_float(from_expression);
        u64 value_u64 = (u64)value;
        return LLVMConstInt(to_type_llvm_type, value_u64, false);
    }

    massert(false, "should have found a valid cast");
    return NULL;
}

bool llvm_build_statement_assignment(Statement* statement, LLVM_Scope* scope, LLVM_Function* function) {
    massert(statement->type == statement_assignment, "should have found an assignment");
    LLVMValueRef assignee = llvm_build_expression(&statement->assignment.assignee, scope, function);
    massert(statement->assignment.assignee.type.is_ref, "should have found a ref to assign to");
    LLVMValueRef value = llvm_build_expression(&statement->assignment.value, scope, function);
    LLVMBuildStore(cap_context.llvm_info.builder, value, assignee);
    return false;
}

bool llvm_build_statement_return(Statement* statement, LLVM_Scope* scope, LLVM_Function* function) {
    massert(statement->type == statement_return, "should have found a return");
    LLVMValueRef value = llvm_build_expression(&statement->return_.value, scope, function);
    LLVMBuildRet(cap_context.llvm_info.builder, value);
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

LLVM_Function llvm_get_function(Templated_Function* templated_function) {
    for (u32 i = 0; i < cap_context.llvm_functions.count; i++) {
        LLVM_Function_Pair* pair = LLVM_Function_Pair_List_get(&cap_context.llvm_functions, i);
        Templated_Function* pair_templated_function = pair->templated_function;
        if (pair_templated_function == templated_function) {
            return *pair->function;
        }
    }
    massert(false, "should have found a function");
    return (LLVM_Function){0};
}

void llvm_compile_program(Program* program, char* build_dir) {
    LLVMTypeRef llvm_return_type = llvm_get_type(&program->body.return_type);
    LLVMTypeRef function_type = LLVMFunctionType(llvm_return_type, NULL, 0, false);
    LLVMValueRef function_value = LLVMAddFunction(cap_context.llvm_info.module, "main", function_type);

    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(function_value, "entry");
    LLVM_Function function = {0};
    function.return_type = llvm_return_type;
    function.function_value = function_value;
    function.templated_function = program->body.templated_functions.data[0];
    LLVMPositionBuilderAtEnd(cap_context.llvm_info.builder, entry_block);

    LLVM_Scope llvm_scope = llvm_compile_scope(&function.templated_function->body, &function);
    if (!llvm_scope.has_exit) {
        LLVMValueRef zero = LLVMConstInt(llvm_get_type(&program->body.return_type), 0, 0);
        LLVMBuildRet(cap_context.llvm_info.builder, zero);
        llvm_scope.has_exit = true;
    }

    LLVMPositionBuilderAtEnd(cap_context.llvm_info.builder, entry_block);
    LLVMBuildBr(cap_context.llvm_info.builder, llvm_scope.block);

    // ############# DONE building llvm ir #############
    char* error;
    char* ir = LLVMPrintModuleToString(cap_context.llvm_info.module);
    printf("\nLLVM IR:\n%s\n\n", ir);
    LLVMDisposeMessage(ir);

    if (LLVMVerifyModule(cap_context.llvm_info.module, LLVMAbortProcessAction, &error) != 0) {
        red_printf("LLVMVerifyModule failed: %s\n", error);
        LLVMDisposeMessage(error);
        return;
    } else {
        green_printf("Successfully verified module\n");
    }

    char* obj_path = alloc(8192);
    snprintf(obj_path, 8192, "%s/%s.obj", build_dir, program->name);

    if (LLVMTargetMachineEmitToFile(cap_context.llvm_info.target_machine, cap_context.llvm_info.module, obj_path, LLVMObjectFile, &error) != 0) {
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
        printf("\n\nProgram output:\n");
        char buffer[512 * 8];
        sprintf(buffer, "./%s.exe", program->name);
        int res = system(exe_path);
        printf("\n\n");
        printf("Program exited with code: %d\n", res);
    }
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
        case expression_variable:
        case expression_cast:
        case expression_invalid: {
            massert(false, "should never happen");
            return 0.0;
        }
    }
}
