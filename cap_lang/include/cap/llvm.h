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
#include <llvm-c/Object.h>
#include <llvm-c/Support.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>

#include "cap/semantic.h"

typedef struct LLVM_Variable_Pair {
    Variable* variable;
    LLVMValueRef value;
} LLVM_Variable_Pair;

typedef struct LLVM_Scope LLVM_Scope;
typedef struct LLVM_Scope {
    Scope* scope;
    LLVM_Variable_Pair_List variables;
    LLVMBasicBlockRef block;
    bool has_exit;
    LLVM_Scope* parent;
} LLVM_Scope;

typedef struct LLVM_Function {
    LLVMValueRef function_value;
    LLVMTypeRef function_type;
    Templated_Function* templated_function;
    Allocator_Connection_Map allocator_connection_map;
} LLVM_Function;

typedef struct LLVM_Context {
    LLVMContextRef llvm_context;
    LLVMBuilderRef builder;
    LLVMTargetDataRef data_layout;
    LLVMModuleRef module;
    LLVMTargetMachineRef target_machine;
    LLVM_Function_List llvm_functions;
    LLVM_Function malloc_function;
} LLVM_Context;

extern LLVM_Context llvm_context;

void llvm_setup_program_context();
void llvm_cleanup_program_context();

LLVMTypeRef llvm_get_type(Type* type);

void llvm_compile_program(Program* program, char* build_dir);

LLVM_Scope llvm_compile_scope(Scope* scope, LLVM_Function* function, LLVM_Scope* parent);

LLVMValueRef llvm_variable_to_llvm(LLVM_Scope* scope, Variable* variable);

void llvm_store_variable_llvm_value(LLVM_Scope* scope, Variable* variable, LLVMValueRef value);

LLVM_Function* llvm_get_function(Templated_Function* templated_function, LLVM_Function* function_getting_this, Expression_List* parameters, Type* return_type);

bool llvm_build_statement(Statement* statement, LLVM_Scope* scope, LLVM_Function* function);

bool llvm_build_statement_assignment(Statement* statement, LLVM_Scope* scope, LLVM_Function* function);

bool llvm_build_statement_return(Statement* statement, LLVM_Scope* scope, LLVM_Function* function);

bool llvm_build_statement_expression(Statement* statement, LLVM_Scope* scope, LLVM_Function* function);

bool llvm_build_statement_variable_declaration(Statement* statement, LLVM_Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_build_expression(Expression* expression, LLVM_Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_build_expression_int(Expression* expression, LLVM_Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_build_expression_float(Expression* expression, LLVM_Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_build_expression_variable(Expression* expression, LLVM_Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_build_expression_cast(Expression* expression, LLVM_Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_build_expression_ptr(Expression* expression, LLVM_Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_build_expression_get(Expression* expression, LLVM_Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_build_expression_alloc(Expression* expression, LLVM_Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_build_expression_function_call(Expression* expression, LLVM_Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_build_expression_type(Expression* expression, LLVM_Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_build_expression_type_size(Expression* expression, LLVM_Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_build_expression_type_align(Expression* expression, LLVM_Scope* scope, LLVM_Function* function);

char* llvm_evaluate_const_int(Expression* expression);

double llvm_evaluate_const_float(Expression* expression);
