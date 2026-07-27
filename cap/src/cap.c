#include "cap.h"

#include "ast.c"
#include "llvm.c"
#include "log.c"
#include "project.c"
#include "ssa.c"
#include "ssa.h"
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

    ast_setup_intrinsics();

    context.evaluate_context = alloc(sizeof(Evaluate_Context));
    Function_Context* global_function_context = alloc(sizeof(Function_Context));
    All_Function_Context global_all = {0};
    global_all.function_context = global_function_context;
    ssa_push_function_context(global_all);

    SSA_Block* global_block = &context.global_block;
    global_block->kind = SSA_Block_Kind_Global;

    ssa_init_intrinsic_block();

    return 0;
}

Cap_Folder cap_analyze(utf8 path) {
    return cap_folder_create_from_path(path);
}
