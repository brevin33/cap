#include "cap.h"

Cap_Context cap_context;

Cap_Project cap_create_project(String path) {
    Cap_Project project = {0};
    project.base_folder = cap_create_folder(path);
    return project;
}

Cap_Folder cap_create_folder(String path) {
    Cap_Folder folder = {0};
    folder.path = path;

    u64 files_capacity = 8;
    folder.files_count = 0;
    folder.files = cap_alloc(files_capacity * sizeof(Cap_File));

    u64 files_in_dir_count = 0;
    String* files_in_dir = filesystem_read_files_in_folder(path, &files_in_dir_count);
    for (u64 i = 0; i < files_in_dir_count; i++) {
        String file_path = string_append(folder.path, str("/"));
        file_path = string_append(file_path, files_in_dir[i]);
        if (file_path.length < 4) continue;
        String last4_char = string_create(file_path.data + file_path.length - 4, 4);
        if (!string_equal(last4_char, str(".cap"))) continue;
        Cap_File file = cap_create_file(file_path);
        ptr_append(folder.files, folder.files_count, files_capacity, file);
    }

    u64 functions_capacity = 8;
    folder.function_ids = cap_alloc(functions_capacity * sizeof(u64));
    folder.functions_count = 0;

    u64 programs_capacity = 8;
    folder.programs = cap_alloc(programs_capacity * sizeof(Program));
    folder.programs_count = 0;
    for (u64 i = 0; i < folder.files_count; i++) {
        Cap_File* file = &folder.files[i];
        Ast* ast = &file->ast;
        for (u64 j = 0; j < ast->top_level.functions_count; j++) {
            Ast* function_ast = &file->ast.top_level.functions[j];
            Function function = sem_function_parse(function_ast);
            Function* function_ptr = sem_add_function_to_scope(&function, &cap_context.global_scope);
            ptr_append(folder.function_ids, folder.functions_count, functions_capacity, function_ptr);
        }

        for (u64 j = 0; j < ast->top_level.programs_count; j++) {
            Ast* program_ast = &file->ast.top_level.programs[j];
            Program program = sem_program_parse(program_ast);
            ptr_append(folder.programs, folder.programs_count, programs_capacity, program);
        }
    }

    return folder;
}

Cap_File cap_create_file(String path) {
    Cap_File file = {0};

    file.path = path;
    file.content = filesystem_read_file(path);
    file.tokens = token_tokenize(&file);
    file.ast = ast_parse_tokens(file.tokens, &file);

    return file;
}

void* cap_alloc(u64 size) {
    Arena* arena = &cap_context.arena;
    return arena_alloc(arena, size);
}

void cap_init() {
    cap_context.arena = arena_create(1024 * 1024, NULL);
    cap_context.log = true;
}
