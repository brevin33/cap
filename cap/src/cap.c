#include "cap.h"

#include "ast.c"
#include "log.c"
#include "project.c"
#include "ssa.c"
#include "token.c"
#include "util/util.c"

void* alloc(u64 size) {
    size += 1024;
    void* mem = arena_alloc(&context.arena, size);
    memset(mem, 0, size);
    return mem;
}

int cap_init(utf8 dir) {
    context.arena = arena_init(MB(4), NULL);
    FILE* file = NULL;
    errno_t err = fopen_s(&file, "cap_compiler_log.txt", "w");
    if (err != 0 || file == NULL) {
        printf("Failed to open log file\n");
    }
    context.log_file = file;
    int res = _chdir(dir.data);
    if (res != 0) {
        printf("Failed to change directory to: %.*s\n", utf8_fmt(dir));
        return -1;
    }
    context.log_print = true;
    context.pointer_providence_counter = UINT32_MAX;
    context.ssa_evaluation_context = alloc(sizeof(SSA_Evaluation_Context));

    Function_Context* function_context = alloc(sizeof(Function_Context));
    ptr_append(context.ssa_evaluation_context->function_context_stack, context.ssa_evaluation_context->function_context_stack_count,
               context.ssa_evaluation_context->function_context_stack_capacity, function_context);

    ast_setup_intrinsics();

    ssa_init_intrinsic_block();

    SSA_Block* intrinsic_block = &context.intrinsic_ssa_block.block;
    intrinsic_block->kind = SSA_Block_Kind_Global;
    SSA_Block* global_block = &context.global_block;
    global_block->kind = SSA_Block_Kind_Global;
    ptr_append(global_block->branchs_to_this_block, global_block->branchs_to_this_block_count, global_block->branchs_to_this_block_capacity, intrinsic_block);

    return 0;
}

Cap_Folder cap_analyze(utf8 path) {
    return cap_folder_create_from_path(path);
}
