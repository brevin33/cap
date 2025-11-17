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

typedef struct LLVM_Scope {
    Scope* scope;
    LLVM_Variable_Pair_List variables;
    LLVMBasicBlockRef block;
    bool has_exit;
} LLVM_Scope;

typedef struct LLVM_Function {
    LLVMTypeRef return_type;
    LLVMValueRef function_value;
    Templated_Function* templated_function;
} LLVM_Function;

typedef struct LLVM_Function_Pair {
    LLVM_Function* function;
    Templated_Function* templated_function;
} LLVM_Function_Pair;

LLVMTypeRef llvm_get_type(Type* type);

void llvm_compile_program(Program* program, char* build_dir);

LLVM_Scope llvm_compile_scope(Scope* scope, LLVM_Function* function);

LLVMValueRef llvm_variable_to_llvm(LLVM_Scope* scope, Variable* variable);

void llvm_store_variable_llvm_value(LLVM_Scope* scope, Variable* variable, LLVMValueRef value);

LLVM_Function llvm_get_function(Templated_Function* templated_function);

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

char* llvm_evaluate_const_int(Expression* expression);

double llvm_evaluate_const_float(Expression* expression);
