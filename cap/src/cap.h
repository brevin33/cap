#pragma once

#include "ast.h"
#include "log.h"
#include "project.h"
#include "ssa.h"
#include "token.h"
#include "util/util.h"

typedef struct Context Context;
typedef struct Intrinsic_SSA_Block Intrinsic_SSA_Block;

struct Intrinsic_SSA_Block {
    SSA_Block block;

    SSA* function_type;
    void* function_type_value;

    SSA* type_type;
    void* type_type_value;

    SSA* int_literal_type;
    void* int_literal_type_value;

    SSA* float_literal_type;
    void* float_literal_type_value;

    SSA* void_type;
    void* void_type_value;

    SSA* int_type;
    void* int_type_value;

    SSA* uint_type;
    void* uint_type_value;

    SSA* float_type;
    void* float_type_value;

    SSA* compile_to_llvm_ir;
    void* compile_to_llvm_ir_value;

    SSA* call_setup_type;
    void* call_setup_type_value;
};

struct Context {
    Arena arena;

    Log* logs;
    u32 logs_count;
    u32 logs_capacity;

    Scope intrinsic_scope;
    Intrinsic_SSA_Block intrinsic_ssa_block;
    SSA_Block global_block;

    SSA_Evaluation_Context* ssa_evaluation_context;

    u64 pointer_providence_counter;

    FILE* log_file;
    bool log_print;
};

extern Context context;

void* alloc(u64 size);

int cap_init(utf8 dir);

Cap_Folder cap_analyze(utf8 path);
