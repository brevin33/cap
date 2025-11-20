#include "cap.h"

Project* project_create(const char* dir_path) {
    Project* project = alloc(sizeof(Project));
    project->dir_path = alloc(strlen(dir_path) + 1);
    memcpy(project->dir_path, dir_path, strlen(dir_path) + 1);
    Project_Ptr_List_add(&cap_context.projects, &project);

    u64 file_in_directory_count = 0;
    get_file_in_directory(dir_path, &file_in_directory_count);
    for (u64 i = 0; i < file_in_directory_count; i++) {
        char* file_name = get_file_in_directory(dir_path, &file_in_directory_count)[i];
        char file_path_buffer[512];
        snprintf(file_path_buffer, 512, "%s/%s", dir_path, file_name);
        u32 file_path_length = strlen(file_path_buffer);
        char* file_path = alloc(file_path_length + 1);
        memcpy(file_path, file_path_buffer, file_path_length + 1);

        char* file_extension = get_file_extension(file_path);
        if (strcmp(file_extension, "cap") != 0) {
            continue;
        }
        File* file = file_create(file_path);
        File_Ptr_List_add(&project->files, &file);
    }

    return project;
}

File* file_create(const char* path) {
    File* file = alloc(sizeof(File));
    u32 file_id = cap_context.files.count;
    File_Ptr_List_add(&cap_context.files, &file);

    u32 path_length = strlen(path);
    file->path = alloc(path_length + 1);
    memcpy(file->path, path, path_length + 1);

    file->contents = read_file(path);
    if (file->contents == NULL) {
        log_error(0, 0, 0, "Could not open file %s", path);
        return NULL;
    }
    file->tokens = tokenize(file_id);

    file->ast = ast_from_tokens(file->tokens);

    return file;
}

u32 file_get_line_of_index(File* file, u32 index) {
    char* contents = file->contents;
    u32 line = 1;
    for (u32 i = 0; i < index; i++) {
        if (contents[i] == '\n') {
            line++;
        }
    }
    return line;
}

u32 file_get_front_of_line(File* file, u32 line) {
    u32 current_line = 1;
    u32 index = 0;
    char* contents = file->contents;
    while (current_line < line) {
        while (contents[index] != '\n') {
            index++;
        }
        index++;
        current_line++;
    }
    return index;
}

void project_semantic_analysis(Project* project) {
    if (cap_context.error_count > 0) {
        red_printf("can't continue with semantic analysis because of errors\n");
        return;
    }
    sem_default_setup_types();
    for (u64 i = 0; i < project->files.count; i++) {
        File* file = *File_Ptr_List_get(&project->files, i);
        for (u64 j = 0; j < file->ast.top_level.functions.count; j++) {
            Ast* ast = &file->ast.top_level.functions.data[j];
            Function* function = sem_function_prototype(ast);
            Function_Ptr_List_add(&file->functions, &function);
        }
    }

    for (u64 i = 0; i < project->files.count; i++) {
        File* file = *File_Ptr_List_get(&project->files, i);
        for (u64 j = 0; j < file->ast.top_level.programs.count; j++) {
            Ast* ast = &file->ast.top_level.programs.data[j];
            Program* program = sem_program_parse(ast);
            Program_Ptr_List_add(&file->programs, &program);
        }
    }

    for (u64 i = 0; i < project->files.count; i++) {
        File* file = *File_Ptr_List_get(&project->files, i);
        for (u64 j = 0; j < file->functions.count; j++) {
            Function* function = *Function_Ptr_List_get(&file->functions, j);
            for (u64 k = 0; k < function->templated_functions.count; k++) {
                Templated_Function* templated_function = *Templated_Function_Ptr_List_get(&function->templated_functions, k);
                sem_templated_function_implement(templated_function, function->ast->function_declaration.body);
            }
        }
    }
}

void project_compile_llvm(Project* project) {
    if (cap_context.error_count > 0) {
        red_printf("can't continue with llvm compilation because of errors\n");
        return;
    }
    // compilation starts
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmPrinters();
    LLVMInitializeAllDisassemblers();
    char* triple = LLVMGetDefaultTargetTriple();

    llvm_context.llvm_context = LLVMContextCreate();
    llvm_context.builder = LLVMCreateBuilder();

    char* error;
    LLVMTargetRef target;
    if (LLVMGetTargetFromTriple(triple, &target, &error) != 0) {
        fprintf(stderr, "Failed to get target: %s\n", error);
        LLVMDisposeMessage(error);
        return;
    }

    llvm_context.target_machine = LLVMCreateTargetMachine(target, triple, "", "", LLVMCodeGenLevelDefault, LLVMRelocDefault, LLVMCodeModelDefault);

    llvm_context.data_layout = LLVMCreateTargetDataLayout(llvm_context.target_machine);

    for (u64 i = 0; i < project->files.count; i++) {
        File* file = *File_Ptr_List_get(&project->files, i);
        for (u64 j = 0; j < file->programs.count; j++) {
            Program* program = *Program_Ptr_List_get(&file->programs, j);
            char* build_dir = alloc(8192);
            snprintf(build_dir, 8192, "%s/build", project->dir_path);
            delete_directory(build_dir);
            if (!make_directory(build_dir)) {
                red_printf("Could not make build directory: %s\n", build_dir);
            }
            llvm_compile_program(program, build_dir);
        }
    }

    LLVMDisposeBuilder(llvm_context.builder);
    LLVMContextDispose(llvm_context.llvm_context);
}
