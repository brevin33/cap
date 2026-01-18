#include "cap.h"

void llvm_print_module() {
    printf("\n\n LLVM Module:\n");
    char* ir = LLVMPrintModuleToString(cap_context.llvm_info.active_module);
    printf("%s\n", ir);
    printf("---------\n");
    LLVMDisposeMessage(ir);
}

LLVMTypeRef llvm_get_type(Type* type) {
    return NULL;
}

void llvm_add_variable(Variable* variable) {
    LLVMTypeRef variable_type = llvm_get_type(&variable->type);
    String variable_name = variable->name;
    char variable_namec[4096];
    snprintf(variable_namec, 4096, "%.*s", str_info(variable_name));
    LLVMValueRef variable_value = LLVMBuildAlloca(cap_context.llvm_info.builder, variable_type, variable_namec);
    llvm_set_variable_value(variable, variable_value);
}

void llvm_compile_scope(Scope* scope) {
    return llvm_compile_scope_with_initialized_variables(scope, NULL, 0);
}

void llvm_compile_scope_with_initialized_variables(Scope* scope, Variable* scope_variables_already_initalized, u64 scope_variables_already_initalized_count) {
    Scope* last_scope = cap_context.scope;
    cap_context.scope = scope;
    LLVMBasicBlockRef scope_entry_block = LLVMAppendBasicBlock(cap_context.llvm_info.function_being_built, "scope_entry");
    LLVMBasicBlockRef last_block = llvm_set_active_block(scope_entry_block);
    LLVM_Scope_Info* scope_info = llvm_add_scope_info(scope);
    scope_info->entry_block = scope_entry_block;

    for (u64 i = 0; i < scope->variables_count; i++) {
        Variable* variable = scope->variables[i];
        bool already_initalized = false;
        for (u64 j = 0; j < scope_variables_already_initalized_count; j++) {
            Variable* already_initalized_variable = &scope_variables_already_initalized[j];
            if (variable == already_initalized_variable) already_initalized = true;
        }
        if (already_initalized) continue;
        llvm_add_variable(variable);
    }

    LLVMBasicBlockRef scope_statements_block = LLVMAppendBasicBlock(cap_context.llvm_info.function_being_built, "scope_statements");
    llvm_set_active_block(scope_statements_block);
    ptr_append(scope_info->statements_blocks, scope_info->statements_blocks_count, scope_info->statements_blocks_capacity, scope_statements_block);

    for (u64 i = 0; i < scope->statements_count; i++) {
        Statement* statement = &scope->statements[i];
        bool breaks_scope = llvm_compile_statement(statement);
        if (breaks_scope) break;
    }

    llvm_pop_scope_info();
    cap_context.scope = last_scope;
    llvm_set_active_block(last_block);
}

bool llvm_compile_statement(Statement* statement) {
    return false;
}

void llvm_compile_program(Program* program) {
    LLVMModuleRef last_module = cap_context.llvm_info.active_module;
    char cName[2048];
    snprintf(cName, 2048, "%.*s.ll", str_info(program->name));
    LLVMModuleRef program_module = LLVMModuleCreateWithName(cName);
    cap_context.llvm_info.active_module = program_module;

    LLVMTypeRef main_function_type = LLVMFunctionType(LLVMInt32Type(), NULL, 0, false);
    LLVMValueRef main_function_value = LLVMAddFunction(cap_context.llvm_info.active_module, "main", main_function_type);
    cap_context.llvm_info.function_being_built = main_function_value;

    LLVMBasicBlockRef main_entry_block = LLVMAppendBasicBlock(main_function_value, "program_entry");
    llvm_set_active_block(main_entry_block);

    Function* main_function = &program->function;
    Function_Implementation* main_function_implementation = main_function->implementations[0];
    Scope* main_function_scope = &main_function_implementation->body;

    llvm_print_module();

    char* error;
    if (LLVMVerifyModule(cap_context.llvm_info.active_module, LLVMAbortProcessAction, &error) != 0) {
        String error_str = string_create(error, strlen(error));
        String error_message = string_append(str("Failed to verify module: "), error_str);
        mabort(error_message);
    }

    String object_file_name = string_append(program->name, str(".obj"));
    String object_file_path = string_append(cap_context.build_directory, object_file_name);
    char object_file_pathc[4096];
    snprintf(object_file_pathc, 4096, "%.*s", str_info(object_file_path));
    if (LLVMTargetMachineEmitToFile(cap_context.llvm_info.target_machine, program_module, object_file_pathc, LLVMObjectFile, &error) != 0) {
        String error_str = string_create(error, strlen(error));
        String error_message = string_append(str("Failed to emit object file: "), error_str);
        mabort(error_message);
    }

    String exe_file_name = string_append(program->name, str(".exe"));
    String exe_file_path = string_append(cap_context.build_directory, exe_file_name);
    if (!llvm_link_executable(exe_file_path, &object_file_path, 1)) {
        mabort(str("Failed to link executable"));
    };

    LLVMDisposeModule(program_module);
    cap_context.llvm_info.active_module = last_module;
}

bool llvm_link_executable(String exe_file_path, String* object_file_paths, u64 count) {
    massert(count > 0, str("count must be greater than 0"));
    String object_file_paths_joined = object_file_paths[0];
    for (u64 i = 1; i < count; i++) {
        String object_file_path = object_file_paths[i];
        object_file_paths_joined = string_append(object_file_paths_joined, str(" "));
        object_file_paths_joined = string_append(object_file_paths_joined, object_file_path);
    }
    char link_command[4096];
    snprintf(link_command, 4096, "lld-link /OUT:%.*s /SUBSYSTEM:CONSOLE %.*s /DEFAULTLIB:libcmt", str_info(exe_file_path), str_info(object_file_paths_joined));
    return system(link_command) == 0;
}

LLVMBasicBlockRef llvm_set_active_block(LLVMBasicBlockRef block) {
    LLVMBasicBlockRef last_block = cap_context.llvm_info.active_block;
    LLVMPositionBuilderAtEnd(cap_context.llvm_info.builder, block);
    cap_context.llvm_info.active_block = block;
    return last_block;
}

void llvm_set_variable_value(Variable* variable, LLVMValueRef value) {
    Scope* scope = cap_context.scope;
    LLVM_Scope_Info* scope_info = llvm_get_scope_info(scope);
    LLVMValue_Variable_Pair pair = {value, variable};
    ptr_append(scope_info->variable_to_values, scope_info->variable_to_values_count, scope_info->variable_to_values_capacity, pair);
}

LLVMValueRef llvm_get_variable_value(Variable* variable) {
    Scope* scope = cap_context.scope;
    LLVM_Scope_Info* scope_info = llvm_get_scope_info(scope);
    for (u64 i = scope_info->variable_to_values_count - 1; i >= 0; i--) {
        LLVMValue_Variable_Pair pair = scope_info->variable_to_values[i];
        if (pair.variable == variable) return pair.value;
    }
    if (scope->parent != NULL) return llvm_get_variable_value(variable);
    mabort(str("variable not found"));
    return NULL;
}

LLVM_Scope_Info* llvm_get_scope_info(Scope* scope) {
    for (u64 i = cap_context.llvm_info.scope_infos_count - 1; i >= 0; i--) {
        LLVM_Scope_Info_Scope_Pair* pair = &cap_context.llvm_info.scope_infos[i];
        if (pair->scope == scope) return &pair->scope_info;
    }
    mabort(str("scope not found"));
    return NULL;
}

LLVM_Scope_Info* llvm_add_scope_info(Scope* scope) {
    LLVM_Scope_Info scope_info = {0};
    LLVM_Scope_Info_Scope_Pair pair = {scope, scope_info};
    ptr_append(cap_context.llvm_info.scope_infos, cap_context.llvm_info.scope_infos_count, cap_context.llvm_info.scope_infos_capacity, pair);
    return &cap_context.llvm_info.scope_infos[cap_context.llvm_info.scope_infos_count - 1].scope_info;
}

void llvm_pop_scope_info() {
    cap_context.llvm_info.scope_infos_count--;
}
