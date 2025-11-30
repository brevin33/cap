#include "cap.h"
#include "cap/lists.h"
#include "cap/semantic.h"

Project* project_create(const char* dir_path) {
    init_cap_context();

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
        for (u64 j = 0; j < file->ast.top_level.structs.count; j++) {
            Ast* ast = &file->ast.top_level.structs.data[j];
            sem_add_struct(ast);
        }
    }

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

    project_validate_function_call_allocators(project);
}

static bool _project_connectup_id_allocators_to_function_id(u32 calling_id, u32 called_id_match_calling, u32 called_id, u32 calling_id_match_called,
                                                            Templated_Function* called_function, Templated_Function* calling_function, Ast* ast) {
    bool changed = false;
    if (sem_allocator_id_are_connected(called_id_match_calling, called_id, &called_function->allocator_connection_map)) {
        sem_add_allocator_id_connection(&calling_function->allocator_connection_map, calling_id, calling_id_match_called, ast, &changed);
    }
    return changed;
}

static bool _project_connectup_id_allocators_to_function_type(u32 calling_id, u32 called_id_match_calling, Type* called_type, Type* called_type_match_calling,
                                                              Templated_Function* called_function, Templated_Function* calling_function, Ast* ast) {
    bool changed = false;
    if (sem_base_allocator_matters(called_type)) {
        bool change_here =
            _project_connectup_id_allocators_to_function_id(calling_id, called_id_match_calling, called_type->base_allocator_id,
                                                            called_type_match_calling->base_allocator_id, called_function, calling_function, ast);
        changed = changed || change_here;
    }
    for (u32 i = 0; i < called_type->ptr_count; i++) {
        bool change_here =
            _project_connectup_id_allocators_to_function_id(calling_id, called_id_match_calling, called_type->ptr_allocator_ids[i],
                                                            called_type_match_calling->ptr_allocator_ids[i], called_function, calling_function, ast);
        changed = changed || change_here;
    }
    return changed;
}

static bool _project_connectup_id_allocators_to_function(u32 calling_id, u32 called_id_match_calling, Templated_Function* called_function,
                                                         Templated_Function* calling_function, Expression* call_expression, Ast* ast) {
    Scope* called_parameter_scope = &called_function->parameter_scope;
    bool changed = false;
    for (u64 j = 0; j < called_parameter_scope->variables.count; j++) {
        Variable* called_parameter = *Variable_Ptr_List_get(&called_parameter_scope->variables, j);
        Type called_parameter_type = called_parameter->type;
        called_parameter_type = sem_dereference_type(&called_parameter_type);

        Expression* calling_parameter = &call_expression->function_call.parameters.data[j];
        Type calling_parameter_type = calling_parameter->type;

        bool change_here = _project_connectup_id_allocators_to_function_type(calling_id, called_id_match_calling, &called_parameter_type,
                                                                             &calling_parameter_type, called_function, calling_function, ast);
        changed = changed || change_here;
    }

    Type* called_return_type = &called_function->return_type;
    Type* expression_return_type = &call_expression->type;
    bool change_here = _project_connectup_id_allocators_to_function_type(calling_id, called_id_match_calling, called_return_type, expression_return_type,
                                                                         called_function, calling_function, ast);
    changed = changed || change_here;

    return changed;
}

static bool _project_connectup_type_allocators_to_function(Type* type, Type* called_type_match_calling, Templated_Function* called_function,
                                                           Templated_Function* calling_function, Expression* call_expression, Ast* ast) {
    bool changed = false;
    if (sem_base_allocator_matters(type)) {
        u32 allocator_id = type->base_allocator_id;
        u32 called_id_match_calling = called_type_match_calling->base_allocator_id;
        bool change_here =
            _project_connectup_id_allocators_to_function(allocator_id, called_id_match_calling, called_function, calling_function, call_expression, ast);
        changed = changed || change_here;
    }
    for (u32 i = 0; i < type->ptr_count; i++) {
        u32 allocator_id = type->ptr_allocator_ids[i];
        u32 called_id_match_calling = called_type_match_calling->ptr_allocator_ids[i];
        bool change_here =
            _project_connectup_id_allocators_to_function(allocator_id, called_id_match_calling, called_function, calling_function, call_expression, ast);
        changed = changed || change_here;
    }
    return changed;
}

static bool _project_connectup_expression_list_allocators_to_function(Expression* call_expression, Templated_Function* calling_function) {
    massert(call_expression->kind == expression_function_call, "should have found a function call");
    Expression_List parameters = call_expression->function_call.parameters;
    Templated_Function* called_function = call_expression->function_call.templated_function;
    Scope* called_parameter_scope = &called_function->parameter_scope;
    bool changed = false;
    for (u64 i = 0; i < parameters.count; i++) {
        Expression* calling_parameter = Expression_List_get(&parameters, i);
        Type calling_parameter_type = calling_parameter->type;

        Variable* called_scope_var = *Variable_Ptr_List_get(&called_parameter_scope->variables, i);
        Type called_scope_var_type = called_scope_var->type;
        called_scope_var_type = sem_dereference_type(&called_scope_var_type);

        bool changed_here = _project_connectup_type_allocators_to_function(&calling_parameter_type, &called_scope_var_type, called_function, calling_function,
                                                                           call_expression, call_expression->ast);
        changed = changed || changed_here;
    }

    Type* called_return_type = &called_function->return_type;
    Type expression_return_type = call_expression->type;
    bool change_here = _project_connectup_type_allocators_to_function(&expression_return_type, called_return_type, called_function, calling_function,
                                                                      call_expression, call_expression->ast);
    changed = changed || change_here;

    return changed;
}

void project_validate_function_call_allocators(Project* project) {
    bool changed = true;
    Expression_Ptr_List error_expressions = {0};
    while (changed) {
        changed = false;
        for (u64 i = 0; i < project->files.count; i++) {
            File* file = *File_Ptr_List_get(&project->files, i);
            for (u64 j = 0; j < file->functions.count; j++) {
                Function* function = *Function_Ptr_List_get(&file->functions, j);
                for (u64 k = 0; k < function->templated_functions.count; k++) {
                    Templated_Function* calling_function = *Templated_Function_Ptr_List_get(&function->templated_functions, k);
                    Expression_List function_call_expression = calling_function->function_call_expression;
                    for (u64 l = 0; l < function_call_expression.count; l++) {
                        Expression* expression = Expression_List_get(&function_call_expression, l);

                        bool already_found_error = false;
                        for (u64 m = 0; m < error_expressions.count; m++) {
                            Expression* error_expression = *Expression_Ptr_List_get(&error_expressions, m);
                            already_found_error = already_found_error || error_expression == expression;
                        }
                        if (already_found_error) continue;

                        u64 num_errors_pre = cap_context.error_count;
                        bool c = _project_connectup_expression_list_allocators_to_function(expression, calling_function);
                        changed = changed || c;
                        u64 num_errors_post = cap_context.error_count;
                        bool error = num_errors_post > num_errors_pre;
                        if (error) {
                            Expression_Ptr_List_add(&error_expressions, &expression);
                        }
                    }
                }
            }
        }

        for (u64 i = 0; i < project->files.count; i++) {
            File* file = *File_Ptr_List_get(&project->files, i);
            for (u64 j = 0; j < file->programs.count; j++) {
                Program* program = file->programs.data[j];
                Function* function = &program->body;
                for (u64 k = 0; k < function->templated_functions.count; k++) {
                    Templated_Function* calling_function = *Templated_Function_Ptr_List_get(&function->templated_functions, k);
                    Expression_List function_call_expression = calling_function->function_call_expression;
                    for (u64 l = 0; l < function_call_expression.count; l++) {
                        Expression* expression = Expression_List_get(&function_call_expression, l);

                        bool already_found_error = false;
                        for (u64 m = 0; m < error_expressions.count; m++) {
                            Expression* error_expression = *Expression_Ptr_List_get(&error_expressions, m);
                            already_found_error = already_found_error || error_expression == expression;
                        }
                        if (already_found_error) continue;

                        u64 num_errors_pre = cap_context.error_count;
                        bool c = _project_connectup_expression_list_allocators_to_function(expression, calling_function);
                        changed = changed || c;
                        u64 num_errors_post = cap_context.error_count;
                        bool error = num_errors_post > num_errors_pre;
                        if (error) {
                            Expression_Ptr_List_add(&error_expressions, &expression);
                        }
                    }
                }
            }
        }
    }
}

void project_compile_llvm(Project* project) {
    if (cap_context.error_count > 0) {
        red_printf("can't continue with llvm compilation because of errors in semantic analysis\n");
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
