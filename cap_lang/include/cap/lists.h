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
