#include "cap.h"

Cap_Context cap_context;

Cap_Project cap_create_project(String path) {
    if (path.length == 0) mabort(str("path is empty"));
    Cap_Project project = {0};

    cap_init_context(path);
    u64 visisted_namespaces_capacity = 8;
    Cap_Folder** visisted_namespaces = cap_alloc(visisted_namespaces_capacity * sizeof(Cap_Folder*));
    u64 visisted_namespaces_count = 0;
    project.base_folder = cap_create_folder(path, visisted_namespaces, visisted_namespaces_count, visisted_namespaces_capacity);

    return project;
}

Cap_Folder* cap_create_folder(String path, Cap_Folder** visited_folders, u64 visited_folders_count, u64 visited_folders_capacity) {
    if (path.length == 0) mabort(str("path is empty"));
    if ((path.data[path.length - 1] == '/' || path.data[path.length - 1] == '\\') && path.length != 1) path.length -= 1;
    for (u64 i = 0; i < visited_folders_count; i++) {
        Cap_Folder* folder = visited_folders[i];
        if (filesystem_path_are_equal(folder->path, path)) {
            log_error("Circular folder dependency");
            for (u64 j = i; j < visited_folders_count - 1; j++) {
                Cap_Folder* visited_folder = visited_folders[j];
                Cap_Folder* child_folder = visited_folders[j + 1];
                log_info("Folder %.*s includes folder %.*s", str_info(visited_folder->path), str_info(child_folder->path));
            }
            Cap_Folder* last_folder = visited_folders[visited_folders_count - 1];
            log_info("Folder %.*s includes folder %.*s", str_info(last_folder->path), str_info(path));
            mabort(str("Exiting because of circular folder dependency"));
        }
    }
    for (u64 i = 0; i < cap_context.folders_count; i++) {
        Cap_Folder* folder = cap_context.folders[i];
        if (filesystem_path_are_equal(folder->path, path)) {
            return folder;
        }
    }

    Cap_Folder* folder = cap_alloc(sizeof(Cap_Folder));
    folder->path = path;
    folder->namespace_id = cap_context.folders_count;
    ptr_append(cap_context.folders, cap_context.folders_count, cap_context.folders_capacity, folder);
    ptr_append(visited_folders, visited_folders_count, visited_folders_capacity, folder);

    u64 files_capacity = 8;
    folder->files_count = 0;
    folder->files = cap_alloc(files_capacity * sizeof(Cap_File));

    u64 files_in_dir_count = 0;
    String* files_in_dir = filesystem_read_files_in_folder(path, &files_in_dir_count);

    for (u64 i = 0; i < files_in_dir_count; i++) {
        String folder_path_with_slash = string_append(folder->path, str("/"));
        String file_in_dir = files_in_dir[i];
        String file_path = string_append(folder_path_with_slash, files_in_dir[i]);
        if (file_path.length < 4) continue;
        String last4_char = string_create(file_path.data + file_path.length - 4, 4);
        if (!string_equal(last4_char, str(".cap"))) continue;
        Cap_File file = cap_create_file(file_path);
        ptr_append(folder->files, folder->files_count, files_capacity, file);
    }

    u64 folders_capacity = 8;
    u64 folder_namespace_aliases_capacity = 8;
    u64 folder_namespace_aliases_count = 0;
    folder->folder_namespace_aliases = cap_alloc(folder_namespace_aliases_capacity * sizeof(String));
    folder->folders = cap_alloc(folders_capacity * sizeof(Cap_Folder*));
    folder->folders_count = 0;
    for (u64 i = 0; i < folder->files_count; i++) {
        Cap_File* file = &folder->files[i];
        Ast* ast = &file->ast;
        for (u64 j = 0; j < ast->top_level.includes_count; j++) {
            Ast* include_ast = &file->ast.top_level.includes[j];
            massert(include_ast->kind == ast_include, str("expected include"));

            String path_said = include_ast->include.path;
            String path;
            if (!filesystem_path_is_absolute(path_said)) {
                u64 len = path_said.length + folder->path.length + 1;
                char* data = cap_alloc(len + 1);
                sprintf(data, "%.*s/%.*s", str_info(folder->path), str_info(path_said));
                path = string_create(data, len);
            } else {
                path = path_said;
            }

            String namespace_alias = include_ast->include.namespace_alias;
            Cap_Folder* child_folder = cap_create_folder(path, visited_folders, visited_folders_count, visited_folders_capacity);
            u64 child_namespace_id = child_folder->namespace_id;
            ptr_append(folder->folders, folder->folders_count, folders_capacity, child_folder);
            ptr_append(folder->folder_namespace_aliases, folder_namespace_aliases_count, folder_namespace_aliases_capacity, namespace_alias);
        }
    }

    cap_context.is_in_semantic_analysis = true;
    cap_context.scope = &cap_context.global_scope;
    cap_context.namespace_we_are_in = folder->namespace_id;

    u64 programs_capacity = 8;
    folder->programs = cap_alloc(programs_capacity * sizeof(Program));
    folder->programs_count = 0;

    u64 global_statements_count = 0;
    for (u64 i = 0; i < folder->files_count; i++) {
        Cap_File* file = &folder->files[i];
        Ast* ast = &file->ast;
        global_statements_count += ast->top_level.top_level_statements_count;
    }

    folder->global_statements = cap_alloc(global_statements_count * sizeof(Statement));
    folder->global_statements_count = 0;

    bool* global_statement_is_complete = cap_alloc(global_statements_count * sizeof(bool));

    // the way that I sort the statements by dependency is by just trying to see if they can be completed then adding them to the list
    // of completed statements. if they can't be completed then they are invalid and we just skip them until the next iteration where more statements
    // have been completed so they might now be able to be completed
    cap_context.log = false;
    u64 last_global_statements_count = INT64_MAX;
    while (last_global_statements_count != folder->global_statements_count) {
        last_global_statements_count = folder->global_statements_count;
        u64 statement_index = 0;
        for (u64 i = 0; i < folder->files_count; i++) {
            Cap_File* file = &folder->files[i];
            Ast* ast = &file->ast;
            massert(ast->kind == ast_top_level, str("expected ast_top_level"));
            for (u64 j = 0; j < ast->top_level.top_level_statements_count; j++) {
                bool is_complete = global_statement_is_complete[statement_index];
                if (is_complete) {
                    statement_index += 1;
                    continue;
                }

                Ast* top_level_statement_ast = &ast->top_level.top_level_statements[j];
                Statement top_level_statement = sem_statement_parse(top_level_statement_ast);
                if (top_level_statement.kind == statement_invalid) {
                    statement_index += 1;
                    continue;
                }
                folder->global_statements[folder->global_statements_count] = top_level_statement;
                global_statement_is_complete[statement_index] = true;
                folder->global_statements_count += 1;
                statement_index += 1;
            }
        }
    }
    cap_context.log = true;

    if (folder->global_statements_count != global_statements_count) {
        u64 statement_index = 0;
        for (u64 i = 0; i < folder->files_count; i++) {
            Cap_File* file = &folder->files[i];
            Ast* ast = &file->ast;
            massert(ast->kind == ast_top_level, str("expected ast_top_level"));
            for (u64 j = 0; j < ast->top_level.top_level_statements_count; j++) {
                Ast* top_level_statement_ast = &ast->top_level.top_level_statements[j];
                bool is_complete = global_statement_is_complete[statement_index];
                if (!is_complete) {
                    // call to log errors
                    Statement top_level_statement = sem_statement_parse(top_level_statement_ast);
                    massert(top_level_statement.kind == statement_invalid, str("should be invalid or it would have been completed"));
                }
                statement_index += 1;
            }
        }
    }

    for (u64 i = 0; i < folder->files_count; i++) {
        Cap_File* file = &folder->files[i];
        Ast* ast = &file->ast;
        for (u64 j = 0; j < ast->top_level.programs_count; j++) {
            Ast* program_ast = &file->ast.top_level.programs[j];
            Program program = sem_program_parse(program_ast);
            ptr_append(folder->programs, folder->programs_count, programs_capacity, program);
        }
    }

    for (; cap_context.expression_to_complete_index < cap_context.expression_to_complete_count; cap_context.expression_to_complete_index++) {
        Expression* expr = cap_context.expression_to_complete[cap_context.expression_to_complete_index];
        sem_complete_expression(expr);
    }

    if (cap_context.error_count > 0) return folder;

    cap_context.is_in_semantic_analysis = false;
    for (u64 i = 0; i < folder->programs_count; i++) {
        Program* program = &folder->programs[i];
        llvm_compile_program(program);
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
    Arena* arena = cap_context.active_arena;
    void* mem = arena_alloc(arena, size);
    memset(mem, 0, size);
    return mem;
}

void cap_init_context(String path) {
    cap_context.global_arena = arena_create(MB(100), NULL);
    cap_context.active_arena = &cap_context.global_arena;
    cap_context.log = true;
    cap_context.folders_capacity = 8;
    cap_context.folders = cap_alloc(cap_context.folders_capacity * sizeof(Cap_Folder*));
    cap_context.folders_count = 0;

    cap_context.expression_to_complete_capacity = 8;
    cap_context.expression_to_complete_count = 0;
    cap_context.expression_to_complete = cap_alloc(cap_context.expression_to_complete_capacity * sizeof(Expression*));
    cap_context.expression_to_complete_index = 0;

    cap_context.implementation_to_complete_recursion_counter = 0;

    cap_context.global_scope = (Scope){0};
    cap_context.global_scope.variables_capacity = 8;
    cap_context.global_scope.variables_count = 0;
    cap_context.global_scope.variables = cap_alloc(cap_context.global_scope.variables_capacity * sizeof(Variable*));
    cap_context.global_scope.statements_capacity = 8;
    cap_context.global_scope.statements_count = 0;
    cap_context.global_scope.statements = cap_alloc(cap_context.global_scope.statements_capacity * sizeof(Statement*));
    cap_context.global_scope.parent = NULL;

    cap_context.scope = &cap_context.global_scope;

    cap_context.is_in_semantic_analysis = false;

    Type* type = cap_alloc(sizeof(Type));
    *type = sem_void_type();
    Variable* void_var = sem_add_variable(str("void"), sem_type_type(), NULL);
    Type** type_ptr = cap_alloc(sizeof(Type*));
    *type_ptr = type;
    sem_set_variable_compile_time_value(void_var, type_ptr);

    type = cap_alloc(sizeof(Type));
    *type = sem_type_type();
    Variable* type_var = sem_add_variable(str("type"), sem_type_type(), NULL);
    type_ptr = cap_alloc(sizeof(Type*));
    *type_ptr = type;
    sem_set_variable_compile_time_value(type_var, type_ptr);

    for (u64 i = 0; i < 256; i++) {
        String number_str = string_int(i);

        type = cap_alloc(sizeof(Type));
        *type = sem_int_type(i);
        Variable* int_var = sem_add_variable(string_append(str("i"), number_str), sem_type_type(), NULL);
        type_ptr = cap_alloc(sizeof(Type*));
        *type_ptr = type;
        sem_set_variable_compile_time_value(int_var, type_ptr);

        type = cap_alloc(sizeof(Type));
        *type = sem_float_type(i);
        Variable* float_var = sem_add_variable(string_append(str("f"), number_str), sem_type_type(), NULL);
        type_ptr = cap_alloc(sizeof(Type*));
        *type_ptr = type;
        sem_set_variable_compile_time_value(float_var, type_ptr);

        type = cap_alloc(sizeof(Type));
        *type = sem_uint_type(i);
        Variable* uint_var = sem_add_variable(string_append(str("u"), number_str), sem_type_type(), NULL);
        type_ptr = cap_alloc(sizeof(Type*));
        *type_ptr = type;
        sem_set_variable_compile_time_value(uint_var, type_ptr);
    }

    if (path.data[path.length - 1] == '/' || path.data[path.length - 1] == '\\') {
        cap_context.build_directory = string_append(path, str("build/"));
    } else {
        cap_context.build_directory = string_append(path, str("/build/"));
    }
    if (filesystem_directory_exists(cap_context.build_directory)) {
        if (filesystem_delete_directory(cap_context.build_directory)) mabort(str("Failed to clear old build directory"));
    }
    if (!filesystem_make_directory(cap_context.build_directory)) mabort(str("Failed to create build directory"));

    // llvm context setup
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllDisassemblers();

    char* triple = LLVMGetDefaultTargetTriple();

    cap_context.llvm_info.llvm_context = LLVMContextCreate();
    cap_context.llvm_info.builder = LLVMCreateBuilder();
    cap_context.llvm_info.module_info = (LLVM_Module_Info){0};

    char* error;
    if (LLVMGetTargetFromTriple(triple, &cap_context.llvm_info.target, &error) != 0) {
        String error_str = string_create(error, strlen(error));
        String error_message = string_append(str("Failed to get llvm target: "), error_str);
        mabort(error_message);
    }

    cap_context.llvm_info.target_machine =
        LLVMCreateTargetMachine(cap_context.llvm_info.target, triple, "", "", LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);
    cap_context.llvm_info.data_layout = LLVMCreateTargetDataLayout(cap_context.llvm_info.target_machine);
}
