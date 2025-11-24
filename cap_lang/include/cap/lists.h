#pragma once
#include "base.h"

typedef struct File File;
create_list_headers(File_Ptr_List, File *);

typedef struct Project Project;
create_list_headers(Project_Ptr_List, Project *);

typedef struct Ast Ast;
create_list_headers(Ast_List, Ast);

typedef struct Function_Parameter Function_Parameter;
create_list_headers(Function_Parameter_List, Function_Parameter);

typedef struct Type_Base Type_Base;
create_list_headers(Type_Base_Ptr_List, Type_Base *);

typedef struct Function Function;
create_list_headers(Function_Ptr_List, Function *);

typedef struct Variable Variable;
create_list_headers(Variable_Ptr_List, Variable *);

typedef struct Allocator Allocator;
create_list_headers(Allocator_List, Allocator);

typedef struct Templated_Function Templated_Function;
create_list_headers(Templated_Function_Ptr_List, Templated_Function *);

typedef struct Statement Statement;
create_list_headers(Statement_List, Statement);

create_list_headers(u32_List, u32);

create_list_headers(u32_List_List, u32_List);

typedef struct Program Program;
create_list_headers(Program_Ptr_List, Program *);

typedef struct LLVM_Variable_Pair LLVM_Variable_Pair;
create_list_headers(LLVM_Variable_Pair_List, LLVM_Variable_Pair);

typedef struct LLVM_Function LLVM_Function;
create_list_headers(LLVM_Function_List, LLVM_Function);

typedef struct Expression Expression;
create_list_headers(Expression_List, Expression);

typedef struct LLVMOpaqueType *LLVMTypeRef;
create_list_headers(LLVMTypeRef_List, LLVMTypeRef);

typedef struct Struct_Field Struct_Field;
create_list_headers(Struct_Field_List, Struct_Field);

typedef struct Struct_Field_Allocator_Info Struct_Field_Allocator_Info;
create_list_headers(Struct_Field_Allocator_Info_List, Struct_Field_Allocator_Info);
