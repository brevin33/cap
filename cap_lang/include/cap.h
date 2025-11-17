#pragma once
#include "cap/ast.h"
#include "cap/lists.h"
#include "cap/llvm.h"
#include "cap/log.h"
#include "cap/project.h"
#include "cap/semantic.h"
#include "cap/tokens.h"

typedef struct LLVM_Context_Info {
    LLVMContextRef llvm_context;
    LLVMBuilderRef builder;
    LLVMTargetDataRef data_layout;
    LLVMModuleRef module;
    LLVMTargetMachineRef target_machine;
} LLVM_Context_Info;

typedef struct Cap_Context {
    File_Ptr_List files;
    Project_Ptr_List projects;
    Type_Base_Ptr_List types;
    Function_Ptr_List functions;
    LLVM_Context_Info llvm_info;
    LLVM_Function_Pair_List llvm_functions;
} Cap_Context;

extern Cap_Context cap_context;

Project *project_create(const char *dir_path);

void project_semantic_analysis(Project *project);

void project_compile_llvm(Project *project);
