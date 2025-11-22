#pragma once
#include "cap/ast.h"
#include "cap/lists.h"
#include "cap/llvm.h"
#include "cap/log.h"
#include "cap/project.h"
#include "cap/semantic.h"
#include "cap/tokens.h"

typedef struct Cap_Context {
    File_Ptr_List files;
    Project_Ptr_List projects;
    Type_Base_Ptr_List types;
    Function_Ptr_List functions;
    u64 error_count;
    bool log_errors;
} Cap_Context;

extern Cap_Context cap_context;

void init_cap_context();

Project *project_create(const char *dir_path);

void project_semantic_analysis(Project *project);

void project_compile_llvm(Project *project);
