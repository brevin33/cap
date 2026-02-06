#pragma once
#include <llvm-c/Analysis.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/DebugInfo.h>
#include <llvm-c/Disassembler.h>
#include <llvm-c/Error.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/LLJIT.h>
#include <llvm-c/Object.h>
#include <llvm-c/Support.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>

#include "cap/semantics.h"

typedef struct LLVMValue_Variable_Pair LLVMValue_Variable_Pair;
typedef struct LLVM_Scope_Info LLVM_Scope_Info;
typedef struct LLVM_Scope_Info_Scope_Pair LLVM_Scope_Info_Scope_Pair;
typedef struct LLVM_Function_Info LLVM_Function_Info;
typedef struct LLVM_Function_Info_Function_Implementation_Pair LLVM_Function_Info_Function_Implementation_Pair;
typedef struct LLVM_Module_Info LLVM_Module_Info;

struct LLVM_Scope_Info {
    LLVMBasicBlockRef entry_block;

    LLVMBasicBlockRef* statements_blocks;
    u32 statements_blocks_count;
    u32 statements_blocks_capacity;

    LLVMValue_Variable_Pair* variable_to_values;
    u32 variable_to_values_count;
    u32 variable_to_values_capacity;
};

struct LLVM_Scope_Info_Scope_Pair {
    Scope* scope;
    LLVM_Scope_Info scope_info;
};

struct LLVMValue_Variable_Pair {
    LLVMValueRef value;
    Variable* variable;
};

struct LLVM_Function_Info {
    LLVMValueRef function;
    LLVMTypeRef function_type;
};

struct LLVM_Function_Info_Function_Implementation_Pair {
    Function_Implementation* function_implementation;
    LLVM_Function_Info function_info;
};

void llvm_compile_program(Program* program);
void llvm_print_module();
bool llvm_link_executable(String exe_file_path, String* object_file_paths, u64 count);

LLVMTypeRef llvm_get_type(Type* type);

LLVMBasicBlockRef llvm_set_active_block(LLVMBasicBlockRef block);

LLVM_Scope_Info llvm_compile_scope(Scope* scope);
LLVM_Scope_Info llvm_compile_scope_with_function_variables(Scope* scope, Variable** scope_variables_already_initalized,
                                                           u64 scope_variables_already_initalized_count);

void llvm_set_variable_value(Variable* variable, LLVMValueRef value);
LLVMValueRef llvm_get_variable_value(Variable* variable);

LLVM_Scope_Info* llvm_get_scope_info(Scope* scope);
LLVM_Scope_Info* llvm_add_scope_info(Scope* scope);
void llvm_pop_scope_info();

LLVM_Function_Info* llvm_add_function_info(Function_Implementation* function_implementation);
LLVM_Function_Info* llvm_get_function_info(Function_Implementation* function_implementation);

LLVM_Function_Info* llvm_build_function(Function_Implementation* function_implementation);

// return is if statement breaks out of scope
bool llvm_compile_statement(Statement* statement);
bool llvm_compile_expression_statement(Statement* statement);
bool llvm_compile_assignment_statement(Statement* statement);
bool llvm_compile_return_statement(Statement* statement);

LLVMValueRef llvm_compile_expression(Expression* expression);
LLVMValueRef llvm_compile_cast(Expression* expression);
LLVMValueRef llvm_compile_cast_with_valueref(Expression* expression, LLVMValueRef value_of_cast);
LLVMValueRef llvm_compile_reference(Expression* expression);
LLVMValueRef llvm_compile_function_call(Expression* expression);
LLVMValueRef llvm_compile_dereference(Expression* expression);
LLVMValueRef llvm_compile_multiple_values_access(Expression* expression);
LLVMValueRef llvm_compile_multiple_values_access_with_valueref(Expression* expression, LLVMValueRef value_of_multiple_value);

void llvm_add_variable(Variable* variable);

void* llvm_evaluate_expression(Expression* expression);

// returns the last module
// s
struct LLVM_Module_Info {
    LLVMBasicBlockRef active_block;
    LLVMModuleRef module;
    LLVM_Scope_Info_Scope_Pair* scope_infos;
    u64 scope_infos_count;
    u64 scope_infos_capacity;

    LLVM_Function_Info_Function_Implementation_Pair* function_infos;
    u64 function_infos_count;
    u64 function_infos_capacity;
};
LLVM_Module_Info llvm_create_new_module();
void llvm_set_last_module_info(LLVMModuleRef module);
