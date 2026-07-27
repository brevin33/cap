#pragma once

#include "ast.h"
#include "util/util.h"

typedef struct SSA SSA;

typedef struct SSA_Stack_Alloc SSA_Stack_Alloc;
typedef struct SSA_Store SSA_Store;
typedef struct SSA_Parameter SSA_Parameter;
typedef struct SSA_Argument SSA_Argument;
typedef struct SSA_Function_Declaration SSA_Function_Declaration;
typedef struct SSA_Int_Literal SSA_Int_Literal;
typedef struct SSA_Float_Literal SSA_Float_Literal;
typedef struct SSA_Return SSA_Return;
typedef struct SSA_Load SSA_Load;
typedef struct SSA_Explicit_Cast SSA_Explicit_Cast;
typedef struct SSA_Implicit_Cast SSA_Implicit_Cast;
typedef struct SSA_Int_Type SSA_Int_Type;
typedef struct SSA_Uint_Type SSA_Uint_Type;
typedef struct SSA_Float_Type SSA_Float_Type;
typedef struct SSA_Build SSA_Build;
typedef struct SSA_Call_Setup SSA_Call_Setup;
typedef struct SSA_Call SSA_Call;
typedef struct SSA_Call_Return_Type SSA_Call_Return_Type;
typedef struct SSA_Pointer_Type SSA_Pointer_Type;
typedef struct SSA_Underlying_Type SSA_Underlying_Type;
typedef struct SSA_Argument_Type SSA_Argument_Type;
typedef struct SSA_Parameter_Type SSA_Parameter_Type;
typedef struct SSA_Default_Value SSA_Default_Value;
typedef struct SSA_Struct_Type SSA_Struct_Type;
typedef struct SSA_Struct_Value SSA_Struct_Value;
typedef struct SSA_Struct_Index_Number SSA_Struct_Index_Number;
typedef struct SSA_Struct_Type_Index_Number SSA_Struct_Type_Index_Number;
typedef struct SSA_Struct_Index_Name SSA_Struct_Index_Name;
typedef struct SSA_Struct_Type_Index_Name SSA_Struct_Type_Index_Name;

typedef union SSA_Data SSA_Data;
typedef struct SSA_Per_Function_Context_Values SSA_Per_Function_Context_Values;

typedef struct SSA_Block SSA_Block;
typedef union SSA_Block_Data SSA_Block_Data;

typedef struct SSA_Block_Function_Setup SSA_Block_Function_Setup;
typedef struct SSA_Block_Function SSA_Block_Function;

typedef struct SSA_List SSA_List;

typedef struct Function Function;

typedef struct Function_Internal Function_Internal;
typedef struct Function_Intrinsic Function_Intrinsic;

typedef union Function_Data Function_Data;

typedef struct Type Type;
typedef union Type_Data Type_Data;

typedef struct Type_Int Type_Int;
typedef struct Type_Uint Type_Uint;
typedef struct Type_Float Type_Float;
typedef struct Type_Ptr Type_Ptr;
typedef struct Type_Struct Type_Struct;
typedef struct Type_Optional Type_Optional;

typedef struct Allocator Allocator;

typedef struct Allocator_Value Allocator_Value;

typedef struct Allocator_Constraint Allocator_Constraint;
typedef union Allocator_Constraint_Data Allocator_Constraint_Data;
typedef struct Allocator_Constraint_Function_Parameter Allocator_Constraint_Function_Parameter;
typedef struct Allocator_Constraint_Function_Return Allocator_Constraint_Function_Return;

typedef struct Function_Context Function_Context;
typedef struct Evaluate_Context Evaluate_Context;

typedef struct Interpreter_Function_Context Interpreter_Function_Context;
typedef struct Interpreter_Stack_Alloc_Pair Interpreter_Stack_Alloc_Pair;

typedef struct All_Function_Context All_Function_Context;

typedef struct Infer_Context Infer_Context;

typedef struct Log Log;

typedef enum Allocator_Value_Kind {
    Allocator_Value_Kind_Invalid = 0,
    Allocator_Value_Kind_Unspecified,
    Allocator_Value_Kind_Unknown,
    Allocator_Value_Kind_SSA,
    Allocator_Value_Kind_Global,
    Allocator_Value_Kind_Stack,
} Allocator_Value_Kind;

struct Allocator_Value {
    Allocator_Value_Kind kind;
    SSA* ssa;  // NULL if global or stack
};

typedef enum Allocator_Constraint_Kind {
    Allocator_Constraint_Kind_Invalid = 0,
    Allocator_Constraint_Kind_Function_Parameter,
    Allocator_Constraint_Kind_Function_Return,
} Allocator_Constraint_Kind;

struct Allocator_Constraint_Function_Parameter {
    Function_Context* function_context;
    u32 parameter_index;
    u32 parameter_allocator_index;
};

struct Allocator_Constraint_Function_Return {
    Function_Context* function_context;
    u32 return_index;
    u32 return_allocator_index;
};

union Allocator_Constraint_Data {
    Allocator_Constraint_Function_Parameter function_parameter;
    Allocator_Constraint_Function_Return function_return;
};

struct Allocator_Constraint {
    Allocator_Constraint_Kind kind;
    Allocator_Constraint_Data data;
};

struct Allocator {
    Allocator_Value value;
    Allocator_Constraint* constraints;
    u32 constraints_count;
    u32 constraints_capacity;
};

typedef enum Type_Kind {
    Type_Kind_Invalid = 0,
    Type_Kind_Type,
    Type_Kind_Int,
    Type_Kind_Int_Literal,
    Type_Kind_Uint,
    Type_Kind_Float,
    Type_Kind_Float_Literal,
    Type_Kind_Void,
    Type_Kind_Function,
    Type_Kind_Ptr,
    Type_Kind_Call_Setup,
    Type_Kind_Struct,
    Type_Kind_Optional,
} Type_Kind;

struct Type_Struct {
    Type** fields;
    utf8* field_names;
    u32 field_count;
};

struct Type_Int {
    u32 bits;
};

struct Type_Uint {
    u32 bits;
};

struct Type_Float {
    u32 bits;
};

struct Type_Ptr {
    Type* type;
    Allocator** allocator;
};

struct Type_Optional {
    Type* type;
};

union Type_Data {
    Type_Int int_;
    Type_Uint uint;
    Type_Float float_;
    Type_Ptr ptr;
    Type_Struct struct_;
    Type_Optional optional;
};

struct Type {
    Type_Kind kind;
    Type_Data data;
};

struct SSA_List {
    SSA* statements;
    u32 statements_count;
    u32 statements_capacity;
};

typedef enum SSA_Block_Kind {
    SSA_Block_Kind_Invalid = 0,
    SSA_Block_Kind_Global,
    SSA_Block_Kind_Function_Setup,
    SSA_Block_Kind_Function,
    SSA_Block_Kind_Scope,
} SSA_Block_Kind;

struct SSA_Block_Function_Setup {};

struct SSA_Block_Function {};

union SSA_Block_Data {
    SSA_Block_Function_Setup function_setup;
    SSA_Block_Function function;
};

struct SSA_Block {
    SSA_List* statement_lists;
    u32 statement_lists_count;
    u32 statement_lists_capacity;

    SSA_Block** branchs_to_this_block;
    u32 branchs_to_this_block_count;
    u32 branchs_to_this_block_capacity;

    SSA_Block_Kind kind;
    SSA_Block_Data data;
};

typedef enum Function_Kind {
    Function_Kind_Invalid = 0,
    Function_Kind_Internal,
    Function_Kind_Intrinsic,
} Function_Kind;

struct Function_Internal {
    SSA_Block body;
};

typedef enum Intrinsic_Function_Kind {
    Intrinsic_Function_Kind_Invalid = 0,
    Intrinsic_Function_Int_Type,
    Intrinsic_Function_Uint_Type,
    Intrinsic_Function_Float_Type,
    Intrinsic_Function_Kind_Compile_To_LLVM_IR,
} Intrinsic_Function_Kind;

struct Function_Intrinsic {
    Intrinsic_Function_Kind kind;
};

union Function_Data {
    Function_Internal internal;
    Function_Intrinsic intrinsic;
};

struct Function {
    utf8 name;
    SSA_Block setup_block;
    SSA* return_type;
    SSA** parameters;
    u32 parameter_count;
    Function_Kind kind;
    Function_Data data;
};

typedef enum SSA_Kind {
    SSA_Kind_Invalid = 0,
    SSA_Kind_Store,
    SSA_Kind_Load,
    SSA_Kind_Stack_Alloc,
    SSA_Kind_Function_Type,
    SSA_Kind_Type_Type,
    SSA_Kind_Function_Declaration,
    SSA_Kind_Parameter,
    SSA_Kind_Parameter_Type,
    SSA_Kind_Argument,
    SSA_Kind_Argument_Type,
    SSA_Kind_Int_Literal_Type,
    SSA_Kind_Int_Literal,
    SSA_Kind_Float_Literal_Type,
    SSA_Kind_Float_Literal,
    SSA_Kind_Return,
    SSA_Kind_Return_Type,
    SSA_Kind_Implicit_Cast,
    SSA_Kind_Explicit_Cast,
    SSA_Kind_Int_Type,
    SSA_Kind_Uint_Type,
    SSA_Kind_Float_Type,
    SSA_Kind_Void_Type,
    SSA_Kind_Compile_To_LLVM_IR,
    SSA_Kind_Build,
    SSA_Kind_Call_Setup,
    SSA_Kind_Call_Setup_Type,
    SSA_Kind_Call,
    SSA_Kind_Call_Return_Type,
    SSA_Kind_Pointer_Type,
    SSA_Kind_Underlying_Type,
    SSA_Kind_Default_Value,
    SSA_Kind_Struct_Type,
    SSA_Kind_Struct_Value,
    SSA_Kind_Struct_Index_Number,
    SSA_Kind_Struct_Type_Index_Number,
    SSA_Kind_Struct_Index_Name,
    SSA_Kind_Struct_Type_Index_Name,
} SSA_Kind;

struct SSA_Int_Literal {
    Big_Int value;
};

struct SSA_Float_Literal {
    f64 value;
};

struct SSA_Stack_Alloc {
    SSA* type;
    SSA* initial_value;

    SSA* lost_const_at;  // const if this is NULL
};

struct SSA_Store {
    SSA* value;
    SSA* address;
};

struct SSA_Load {
    SSA* address;
};

struct SSA_Function_Declaration {
    Function function;
};

struct SSA_Parameter {
    u32 index;
};

struct SSA_Parameter_Type {
    u32 index;
};

struct SSA_Argument {
    u32 index;
};

struct SSA_Argument_Type {
    u32 index;
};

struct SSA_Return {
    SSA* return_value;
};

struct SSA_Explicit_Cast {
    SSA* value;
    SSA* type;
};

struct SSA_Implicit_Cast {
    SSA* value;
    SSA* type;
};

struct SSA_Build {
    SSA_Block block;
    Function_Context* function_context;
};

struct SSA_Call_Setup {
    SSA* callee;
    SSA** arguments;
    u32 arguments_count;
};

struct SSA_Call {
    SSA* setup;
};

struct SSA_Call_Return_Type {
    SSA* setup;
};

struct SSA_Pointer_Type {
    SSA* type;
};

struct SSA_Underlying_Type {
    SSA* type;
};

struct SSA_Default_Value {
    SSA* type;
};

struct SSA_Struct_Type {
    SSA** field_types;
    utf8* field_names;
    u32 field_count;
};

struct SSA_Struct_Value {
    SSA** field_values;
    u32 field_count;
};

struct SSA_Struct_Index_Number {
    SSA* struct_value;
    u32 index;
};

struct SSA_Struct_Type_Index_Number {
    SSA* struct_type;
    u32 index;
};

struct SSA_Struct_Index_Name {
    SSA* struct_value;
    utf8 index_name;
};

struct SSA_Struct_Type_Index_Name {
    SSA* struct_type;
    utf8 index_name;
};

union SSA_Data {
    SSA_Stack_Alloc stack_alloc;
    SSA_Store store;
    SSA_Function_Declaration function_declaration;
    SSA_Parameter parameter;
    SSA_Argument argument;
    SSA_Int_Literal int_literal;
    SSA_Float_Literal float_literal;
    SSA_Return return_;
    SSA_Load load;
    SSA_Explicit_Cast explicit_cast;
    SSA_Implicit_Cast implicit_cast;
    SSA_Build build;
    SSA_Call_Setup call_setup;
    SSA_Call call;
    SSA_Call_Return_Type call_return_type;
    SSA_Pointer_Type pointer_type;
    SSA_Underlying_Type underlying_type;
    SSA_Argument_Type argument_type;
    SSA_Parameter_Type parameter_type;
    SSA_Default_Value default_value;
    SSA_Struct_Type struct_type;
    SSA_Struct_Value struct_value;
    SSA_Struct_Index_Number struct_index_number;
    SSA_Struct_Type_Index_Number struct_type_index_number;
    SSA_Struct_Index_Name struct_index_name;
    SSA_Struct_Type_Index_Name struct_type_index_name;
};

#define SSA_SUCCESS_VOID_VALUE ((void*)1)
struct SSA_Per_Function_Context_Values {
    Function_Context* function_context;
    void* value;
    Type type;
    utf8 codegen_name;
    bool evaluated_value;
    bool type_check_result;
    bool type_checked;
    bool codegen_already_has_name;
};

struct SSA {
    SSA_Kind kind;

    SSA_Data data;
    SSA_Block* block;

    SSA* type;

    SSA_Per_Function_Context_Values* ssa_per_function_context_values;
    u32 ssa_per_function_context_values_count;
    u32 ssa_per_function_context_values_capacity;

    Ast* ast;
};

struct Function_Context {
    SSA** arguments;
    SSA** parameters;
    u32 parameters_count;

    SSA* return_type;
};

struct Evaluate_Context {
    All_Function_Context* function_context_stack;
    u32 function_context_stack_count;
    u32 function_context_stack_capacity;
};

struct Interpreter_Stack_Alloc_Pair {
    SSA* ssa;
    void* memory;
};

struct Interpreter_Function_Context {
    Interpreter_Stack_Alloc_Pair* stack_alloc_memory_map;
    u32 stack_alloc_memory_map_count;
    u32 stack_alloc_memory_map_capacity;
};

struct All_Function_Context {
    Function_Context* function_context;
    Interpreter_Function_Context* interpreter_function_context;
};

struct Infer_Context {
    SSA** arguments;
    u32 arguments_count;

    Evaluate_Context* arg_side_evaluate_context;
    Evaluate_Context* decl_side_evaluate_context;

    bool** argument_dependencies;
    u32 argument_dependencies_count;
    u32 argument_dependencies_capacity;
};

SSA* ssa_add_to_block(SSA ssa, SSA_Block* block);
SSA* ssa_ast_to_ssa_non_ref(Ast* ast, SSA_Block* block);
SSA* ssa_ast_to_ssa(Ast* ast, SSA_Block* block);
SSA* ssa_top_level_ast_to_ssa(Ast* ast, SSA_Block* block);
void ssa_top_level_post_parse(SSA* ssa, SSA_Block* block);

void ssa_build_scope(Ast* ast, SSA_Block* block);
void ssa_init_intrinsic_block();

bool ssa_type_check(SSA* ssa);

bool ssa_already_type_checked(SSA* ssa, bool* out_res);
void ssa_cache_type_check(SSA* ssa, bool res, Type* type);
Type* ssa_type(SSA* ssa);

bool ssa_type_check_block(SSA_Block* block);
bool ssa_type_check_builds();

bool ssa_type_check_call(SSA* ssa, Type* type);
bool ssa_type_check_call_int_type(SSA* ssa, Function_Context* function_context, Function* function);
bool ssa_type_check_call_uint_type(SSA* ssa, Function_Context* function_context, Function* function);
bool ssa_type_check_call_float_type(SSA* ssa, Function_Context* function_context, Function* function);
bool ssa_type_check_call_compile_to_llvm_ir(SSA* ssa, Function_Context* function_context, Function* function);

void* ssa_evaluate(SSA* ssa);
Type* ssa_evaluate_type(SSA* ssa);
Function* ssa_evaluate_function(SSA* ssa);
Function_Context* ssa_evaluate_function_context(SSA* ssa);

void ssa_cache_evaluated(SSA* ssa, void* value);
bool ssa_already_evaluated(SSA* ssa, void** out_value);

bool ssa_run(SSA* ssa);
bool ssa_run_block(SSA_Block* block);
bool ssa_run_builds();

SSA* ssa_stack_alloc(SSA* type, SSA* initial_value, SSA_Block* block, Ast* ast);
SSA* ssa_store(SSA* value, SSA* address, SSA_Block* block, Ast* ast);
SSA* ssa_load(SSA* address, SSA_Block* block, Ast* ast);
SSA* ssa_load_if_ref(SSA* value, SSA_Block* block, Ast* ast);
SSA* ssa_parameter(u32 index, SSA_Block* block, Ast* ast);
SSA* ssa_argument(u32 index, SSA_Block* block, Ast* ast);
SSA* ssa_function_declaration(SSA_Block* block, Ast* ast);
SSA* ssa_int_literal(Big_Int value, SSA_Block* block, Ast* ast);
SSA* ssa_float_literal(f64 value, SSA_Block* block, Ast* ast);
SSA* ssa_return(SSA* return_value, SSA_Block* block, Ast* ast);
SSA* ssa_return_type(SSA_Block* block, Ast* ast);
SSA* ssa_implicit_cast(SSA* value, SSA* type, SSA_Block* block, Ast* ast);
SSA* ssa_explicit_cast(SSA* value, SSA* type, SSA_Block* block, Ast* ast);
SSA* ssa_build(SSA_Block* block, Ast* ast);
SSA* ssa_call_setup(SSA* callee, SSA** arguments, u32 arguments_count, SSA_Block* block, Ast* ast);
SSA* ssa_call(SSA* setup, SSA_Block* block, Ast* ast);
SSA* ssa_call_return_type(SSA* setup, SSA_Block* block, Ast* ast);
SSA* ssa_pointer_type(SSA* type, SSA_Block* block, Ast* ast);
SSA* ssa_underlying_type(SSA* type, SSA_Block* block, Ast* ast);
SSA* ssa_argument_type(u32 index, SSA_Block* block, Ast* ast);
SSA* ssa_parameter_type(u32 index, SSA_Block* block, Ast* ast);
SSA* ssa_default_value(SSA* type, SSA_Block* block, Ast* ast);
SSA* ssa_struct_type(SSA** types, utf8* type_names, u32 types_count, SSA_Block* block, Ast* ast);
SSA* ssa_no_field_name_struct_type(SSA** types, u32 types_count, SSA_Block* block, Ast* ast);
SSA* ssa_struct_value(SSA** values, u32 values_count, SSA_Block* block, Ast* ast);
SSA* ssa_struct_index_number(SSA* struct_value, u32 index, SSA_Block* block, Ast* ast);
SSA* ssa_struct_type_index_number(SSA* struct_type, u32 index, SSA_Block* block, Ast* ast);
SSA* ssa_struct_index_name(SSA* struct_value, utf8 index_name, SSA_Block* block, Ast* ast);
SSA* ssa_struct_type_index_name(SSA* struct_type, utf8 index_name, SSA_Block* block, Ast* ast);

SSA* ssa_ast_intrinsic(Ast_Intrinsic intrinsic);

SSA* ssa_function_type();
SSA* ssa_type_type();
SSA* ssa_int_literal_type();
SSA* ssa_float_literal_type();
SSA* ssa_void_type();
SSA* ssa_int_type();
SSA* ssa_uint_type();
SSA* ssa_float_type();
SSA* ssa_compile_to_llvm_ir();
SSA* ssa_call_setup_type();

SSA* ssa_function_declaration_ast_prototype(Ast* ast, SSA_Block* block);
void ssa_function_declaration_ast_implement(SSA* ssa, SSA_Block* block);

SSA* ssa_build_ast_prototype(Ast* ast, SSA_Block* block);
void ssa_build_ast_implement(SSA* ssa, SSA_Block* block);

utf8 ssa_block_to_string(SSA_Block* block);
utf8 ssa_recursive_get_block_strings(SSA_Block* block);

utf8 ssa_get_ssa_block_name(SSA_Block* block);
utf8 ssa_get_ssa_name(SSA* ssa);

utf8 ssa_kind_to_string(SSA_Kind kind);

bool ssa_can_explicit_cast(Type* type, Type* cast_type);
bool ssa_can_implicit_cast(Type* type, Type* cast_type);
void ssa_cast_type_allocator(Type* type, Type* cast_type);
void* ssa_cast_value(void* value, Type* value_type, Type* cast_type);

bool ssa_compile_time_value_equal(void* value1, void* value2, Type* type);

bool ssa_type_equal(Type* type1, Type* type2);
i64 ssa_type_size(Type* type);
i64 ssa_type_size_compile_time(Type* type);
bool ssa_is_math_type(Type* type);
bool ssa_struct_has_field_names(Type* type);

bool ssa_type_allocator_valid(Type* type);
void ssa_allocator_add_constraint(Allocator* allocator, Allocator_Constraint* constraint);
bool ssa_merge_type_allocators(Type* type_1, Type* type_2, SSA* error_ssa);
void ssa_set_all_allocators_to_unknown(Type* type);

void ssa_type_init_allocator(Type* type);

void ssa_type_set_allocator_function_return_full(Type* type);
void ssa_type_set_allocator_function_call_return_full(Type* type, SSA* setup_ssa);
void ssa_type_set_allocator_function_parameter_full(Type* type, u32 parameter_index);

bool ssa_is_ref(SSA* ssa);

bool ssa_infer_arguments(SSA** arguments, SSA** parameters, u32 arguments_count, SSA_Block* setup_block, All_Function_Context function_context, SSA* setup_ssa);

void ssa_add_interpreter_to_function_context();
Interpreter_Function_Context* ssa_clear_interpreter_from_global_context();
void ssa_add_interpreter_to_global_context(Interpreter_Function_Context* inter_function_context);

Function_Context* ssa_get_cache_function_context(SSA* ssa);
All_Function_Context ssa_get_function_context();
void ssa_pop_function_context();

bool ssa_running_interpreter();

void ssa_push_function_context(All_Function_Context all);
