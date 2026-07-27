#pragma once

#include "util/util.h"

typedef struct LLVM_Block LLVM_Block;
typedef struct LLVM_Function LLVM_Function;
typedef struct SSA SSA;
typedef struct SSA_Block SSA_Block;
typedef struct Function Function;

// add to global scope
// @empty = internal global {} zeroinitializer

struct LLVM_Function {
    LLVM_Block* blocks;
    u32 blocks_count;
    u32 blocks_capacity;
};

struct LLVM_Block {
    utf8* statements;
    u32 statements_count;
    u32 statements_capacity;
};

utf8 llvm_ssa_to_name(SSA* ssa);
utf8 llvm_block_to_name(SSA_Block* block, char* buffer);

utf8 llvm_ssa_to_statement(SSA* ssa, utf8* buffer, u32* buffer_capacity);

void llvm_append_assign_empty_value(utf8 name, utf8* buffer, u32* buffer_capacity);

utf8 llvm_global_scope_to_llvm(utf8* buffer, u32* buffer_capacity);

utf8 llvm_block_to_llvm(SSA_Block* block, utf8* buffer, u32* buffer_capacity);
utf8 llvm_block_setup(SSA_Block* block, utf8* buffer, u32* buffer_capacity);
utf8 llvm_block_body(SSA_Block* block, utf8* buffer, u32* buffer_capacity);
