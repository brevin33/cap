#pragma once

#include "token.h"
#include "util/util.h"

typedef struct Ast Ast;
typedef union Ast_Data Ast_Data;

typedef struct Ast_File Ast_File;
typedef struct Ast_Scope Ast_Scope;
typedef struct Ast_Return Ast_Return;
typedef struct Ast_Int Ast_Int;
typedef struct Ast_Float Ast_Float;
typedef struct Ast_Variable Ast_Variable;
typedef struct Ast_Variable_Declaration Ast_Variable_Declaration;
typedef struct Ast_Binary_Operation Ast_Binary_Operation;
typedef struct Ast_Parameter Ast_Parameter;
typedef struct Ast_Parameter_List Ast_Parameter_List;
typedef struct Ast_Function_Declaration Ast_Function_Declaration;
typedef struct Ast_Call Ast_Call;
typedef struct Ast_Argument_List Ast_Argument_List;
typedef struct Ast_Build Ast_Build;
typedef struct Ast_Assign Ast_Assign;
typedef struct Ast_Load Ast_Load;

typedef struct Scope Scope;
typedef struct Scope_Variable Scope_Variable;

typedef struct SSA SSA;

typedef enum Ast_Kind {
    Ast_Kind_Invalid = 0,
    Ast_Kind_File,
    Ast_Kind_Scope,
    Ast_Kind_Return,
    Ast_Kind_Int,
    Ast_Kind_Float,
    Ast_Kind_Variable,
    Ast_Kind_Variable_Declaration,
    Ast_Kind_Binary_Operation,
    Ast_Kind_Parameter,
    Ast_Kind_Parameter_List,
    Ast_Kind_Function_Declaration,
    Ast_Kind_Call,
    Ast_Kind_Argument_List,
    Ast_Kind_Build,
    Ast_Kind_Assign,
    Ast_Kind_Load,
    Ast_Kind_Intrinsic_Int_Type,
    Ast_Kind_Intrinsic_Uint_Type,
    Ast_Kind_Intrinsic_Float_Type,
    Ast_Kind_Intrinsic_Compile_To_LLVM_IR,
    Ast_Kind_Intrinsic_Type,
    Ast_Kind_Intrinsic_Function,
    Ast_Kind_Intrinsic_Void,
} Ast_Kind;

struct Ast_File {
    Ast* top_level_statements;
    u32 ast_count;
};

struct Ast_Build {
    utf8 name;
    Ast* scope;
};

struct Ast_Scope {
    Ast* statements;
    u32 ast_count;

    Scope* scope;  // NULL during parsing. Populated during semantic SSA generation
};

struct Ast_Return {
    Ast* values;
    u32 values_count;
};

struct Ast_Int {
    Big_Int value;
};

struct Ast_Float {
    f64 value;
};

struct Ast_Variable {
    utf8 name;

    Ast* variable_declaration;  // NULL during parsing. Populated during variable resolution
};

struct Ast_Variable_Declaration {
    Ast* type;
    utf8 name;

    SSA* value;  // NULL until semantic SSA generation
};

struct Ast_Binary_Operation {
    Ast* lhs;
    Ast* rhs;
    Token_Kind operator;
};

struct Ast_Parameter {
    Ast* type;
    utf8 name;

    SSA* value;
};

struct Ast_Parameter_List {
    Ast* parameters;
    u32 parameters_count;

    Ast** parameter_semantic_parse_order;

    Scope* scope;  // NULL during parsing. Populated during semantic SSA generation
};

struct Ast_Function_Declaration {
    Ast* return_types;
    u32 return_types_count;
    utf8 name;
    Ast* parameter_list;
    Ast* body;

    SSA* value;  // NULL until semantic SSA generation
};

struct Ast_Call {
    Ast* callee;
    Ast* argument_list;
};

struct Ast_Load {
    Ast* address;
};

struct Ast_Argument_List {
    Ast* arguments;
    u32 arguments_count;
};

struct Ast_Assign {
    Ast* lhs;
    Ast* rhs;
    u32 lhs_count;
    u32 rhs_count;
};

union Ast_Data {
    Ast_File file;
    Ast_Build build;
    Ast_Scope scope;
    Ast_Return return_;
    Ast_Int int_;
    Ast_Float float_;
    Ast_Variable variable;
    Ast_Variable_Declaration variable_declaration;
    Ast_Binary_Operation binary_operation;
    Ast_Parameter parameter;
    Ast_Parameter_List parameter_list;
    Ast_Function_Declaration function_declaration;
    Ast_Call call;
    Ast_Argument_List argument_list;
    Ast_Assign assign;
    Ast_Load load;
};

struct Ast {
    Ast_Kind kind;
    Tokens tokens;
    Cap_File* file;
    Ast_Data data;
    // flags
    bool is_reference;
};

struct Scope_Variable {
    utf8 name;
    Ast* ast;
};

struct Scope {
    Scope* parent;

    Ast* ast;

    Scope_Variable* variables;
    u32 variables_count;
    u32 variables_capacity;
};

#define ast_expect(token, expected_kind)                                                    \
    do {                                                                                    \
        if ((token).kind != (expected_kind)) {                                              \
            char buffer[4096];                                                              \
            utf8 token_type_string = token_kind_to_string((expected_kind));                 \
            snprintf(buffer, sizeof(buffer), "Expected %.*s", utf8_fmt(token_type_string)); \
            u64 len = strlen(buffer);                                                       \
            utf8 msg_utf8 = {0};                                                            \
            msg_utf8.data = alloc(len + 1);                                                 \
            memcpy(msg_utf8.data, buffer, len + 1);                                         \
            msg_utf8.count = len;                                                           \
            log_utf8_token(msg_utf8, log_error, (token), file);                             \
            return (Ast){0};                                                                \
        }                                                                                   \
    } while (0)

Ast ast_create_from_file(Cap_File* file);

utf8 ast_kind_to_string(Ast_Kind kind);

Ast ast_top_level_statement(Tokens* tokens, Cap_File* file);
Ast ast_scoped_statement(Tokens* tokens, Cap_File* file);
Ast ast_statement_starting_with_expression(Tokens* tokens, Cap_File* file);
Ast ast_expression(Tokens* tokens, Cap_File* file);
Ast ast_expression_non_ref(Tokens* tokens, Cap_File* file);

Ast ast_scope(Tokens* tokens, Cap_File* file);
Ast ast_return(Tokens* tokens, Cap_File* file);
Ast ast_int(Tokens* tokens, Cap_File* file);
Ast ast_float(Tokens* tokens, Cap_File* file);
Ast ast_variable(Tokens* tokens, Cap_File* file);
Ast ast_variable_declaration(Tokens* tokens, Cap_File* file, Ast type);
Ast ast_binary_operation(Tokens* tokens, Cap_File* file, Ast* lhs, Ast* rhs, Token_Kind operator);
Ast ast_parameter(Tokens* tokens, Cap_File* file);
Ast ast_parameter_list(Tokens* tokens, Cap_File* file);
Ast ast_function_declaration(Tokens* tokens, Cap_File* file);
Ast ast_call(Tokens* tokens, Cap_File* file, Ast callee);
Ast ast_argument_list(Tokens* tokens, Cap_File* file);
Ast ast_build(Tokens* tokens, Cap_File* file);

bool ast_resolve_variables(Ast* ast, Scope* scope);
bool ast_top_level_prototype_resolve_variables(Ast* ast, Scope* scope);
bool ast_top_level_implement_resolve_variables(Ast* ast, Scope* scope);

Scope_Variable* ast_find_variable_in_scope(Scope* scope, utf8 name, Ast* ast);
Scope_Variable* ast_add_variable_to_scope(Scope* scope, utf8 name, Ast* ast);

Ast ast_load_if_ref(Ast possible_ref);
