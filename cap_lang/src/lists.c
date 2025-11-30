#include "cap.h"

create_list_impl(File_Ptr_List, File *);

create_list_impl(Project_Ptr_List, Project *);

create_list_impl(Ast_List, Ast);

create_list_impl(Function_Parameter_List, Function_Parameter);

create_list_impl(Type_Base_Ptr_List, Type_Base *);

create_list_impl(Function_Ptr_List, Function *);

create_list_impl(Variable_Ptr_List, Variable *);

create_list_impl(Templated_Function_Ptr_List, Templated_Function *);

create_list_impl(Statement_List, Statement);

create_list_impl(Allocator_List, Allocator);

create_list_impl(u32_List, u32);

create_list_impl(u32_List_List, u32_List);

create_list_impl(Program_Ptr_List, Program *);

create_list_impl(LLVM_Variable_Pair_List, LLVM_Variable_Pair);

create_list_impl(LLVM_Function_List, LLVM_Function);

create_list_impl(Expression_List, Expression);

create_list_impl(LLVMTypeRef_List, LLVMTypeRef);

create_list_impl(Struct_Field_Allocator_Info_List, Struct_Field_Allocator_Info);

create_list_impl(Struct_Field_List, Struct_Field);

create_list_impl(Expression_Ptr_List, Expression *);
