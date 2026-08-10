#pragma once

#include "ssa.h"
#include "util/util.h"

typedef struct SSA SSA;
typedef struct SSA_Block SSA_Block;
typedef struct Function Function;
typedef struct Function_Context Function_Context;

utf8 llvm_unique_name();

utf8 llvm_gernerate_ir_exe(Function* function);

utf8 llvm_type(Type* type);
utf8 llvm_ssa_name(SSA* ssa);
utf8 llvm_function_name(Function_Context* function_context);
utf8 llvm_ssa_set_global_name(SSA* ssa);
utf8 llvm_block_name(SSA_Block* block);

void llvm_ssa_set_name(SSA* ssa, utf8 name);
void llvm_ssa_reset_name(SSA* ssa);

bool llvm_type_exists_at_runtime(Type* type);
bool llvm_ssa_exists_at_runtime(SSA* ssa);

utf8 llvm_function(Function* function, Function_Context* function_context, utf8_builder* builder);

utf8 llvm_statement(SSA* ssa, utf8_builder* builder);
utf8 llvm_global_scope(utf8_builder* builder);
utf8 llvm_function_setup(SSA_Block* block, utf8_builder* builder);
utf8 llvm_global_setup(SSA_Block* block, utf8_builder* builder);
utf8 llvm_function_body(SSA_Block* block, utf8_builder* builder);
utf8 llvm_build_required_functions(SSA_Block* block, utf8_builder* builder);

void llvm_branch_to_block(SSA_Block* block, utf8_builder* builder);

void llvm_generate_exe(utf8 llvm_ir, utf8 exe_path);
