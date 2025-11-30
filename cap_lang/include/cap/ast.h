#pragma once
#include "base.h"
#include "cap/lists.h"
#include "cap/tokens.h"

typedef enum Ast_Kind {
    ast_invalid = 0,
    ast_top_level,
    ast_function_declaration,
    ast_declaration_parameter,
    ast_declaration_parameter_list,
    ast_body,
    ast_type,
    ast_variable_declaration,
    ast_assignment,
    ast_return,
    ast_expression,
    ast_int,
    ast_float,
    ast_variable,
    ast_program,
    ast_value_access,
    ast_get,
    ast_ptr,
    ast_alloc,
    ast_function_call,
    ast_function_call_parameter,
    ast_add,
    ast_sub,
    ast_mul,
    ast_div,
    ast_mod,
    ast_bit_and,
    ast_bit_or,
    ast_bit_xor,
    ast_bit_not,
    ast_bit_shl,
    ast_bit_shr,
    ast_and,
    ast_or,
    ast_equals_equals,
    ast_not_equals,
    ast_less_than,
    ast_greater_than,
    ast_less_than_equals,
    ast_greater_than_equals,
    ast_allocator,
    ast_struct,
    ast_struct_body,
    ast_struct_field,
} Ast_Kind;

typedef struct Ast Ast;

typedef struct Ast_Struct {
    char* name;
    Ast* body;
} Ast_Struct;

typedef struct Ast_Struct_Body {
    Ast_List fields;
} Ast_Struct_Body;

typedef struct Ast_Struct_Field {
    Ast* type;
    char* name;
} Ast_Struct_Field;

typedef struct Ast_Alloc {
    Ast* parameters;
    Ast* allocator;
} Ast_Alloc;

typedef struct Ast_Biop {
    Ast* lhs;
    Ast* rhs;
} Ast_Biop;

typedef struct Ast_Get {
    Ast* expression;
} Ast_Get;

typedef struct Ast_Ptr {
    Ast* expression;
} Ast_Ptr;

typedef struct Ast_Allocator {
    char* variable_name;
    char* field_name;
} Ast_Allocator;

typedef struct Ast_Type {
    char* name;
    u64 ptr_count;
    Ast* base_allocator;
    Ast_List ptr_allocators;
} Ast_Type;

typedef struct Ast_Function_Call {
    char* name;
    Ast* parameters;
    Ast* allocator;
} Ast_Function_Call;

typedef struct Ast_Function_Call_Parameter {
    Ast_List parameters;
} Ast_Function_Call_Parameter;

typedef struct Ast_Function_Declaration {
    Ast* return_type;
    char* name;
    Ast* parameter_list;
    Ast* body;
} Ast_Function_Declaration;

typedef struct Ast_Declaration_Parameter {
    Ast* type;
    char* name;
} Ast_Declaration_Parameter;

typedef struct Ast_Declaration_Parameter_List {
    Ast_List parameters;
} Ast_Declaration_Parameter_List;

typedef struct Ast_Body {
    Ast_List statements;
} Ast_Body;

typedef struct Ast_Top_Level {
    Ast_List functions;
    Ast_List programs;
    Ast_List structs;
} Ast_Top_Level;

typedef struct Ast_Return {
    Ast* value;
} Ast_Return;

typedef struct Ast_Assignment {
    Ast* assignee;
    Ast* value;
} Ast_Assignment;

typedef struct Ast_Variable_Declaration {
    Ast* type;
    char* name;
    Ast* assignment;
} Ast_Variable_Declaration;

typedef struct Ast_Variable {
    char* name;
} Ast_Variable;

typedef struct Ast_Expression {
    Ast* value;
} Ast_Expression;

typedef struct Ast_Int {
    char* value;
} Ast_Int;

typedef struct Ast_Float {
    double value;
} Ast_Float;

typedef struct Ast_Program {
    char* name;
    Ast* body;
} Ast_Program;

typedef struct Ast_Value_Access {
    Ast* expr;
    char* access_name;
} Ast_Value_Access;

typedef struct Ast {
    Ast_Kind kind;
    u32 num_tokens;
    Token* token_start;
    union {
        Ast_Top_Level top_level;
        Ast_Function_Declaration function_declaration;
        Ast_Declaration_Parameter_List declaration_parameter_list;
        Ast_Declaration_Parameter declaration_parameter;
        Ast_Body body;
        Ast_Type type;
        Ast_Variable_Declaration variable_declaration;
        Ast_Assignment assignment;
        Ast_Return return_;
        Ast_Expression expression;
        Ast_Int int_;
        Ast_Float float_;
        Ast_Variable variable;
        Ast_Program program;
        Ast_Value_Access value_access;
        Ast_Alloc alloc;
        Ast_Function_Call function_call;
        Ast_Function_Call_Parameter function_call_parameter;
        Ast_Biop biop;
        Ast_Allocator allocator;
        Ast_Get get;
        Ast_Ptr ptr;
        Ast_Struct struct_;
        Ast_Struct_Body struct_body;
        Ast_Struct_Field struct_field;
    };
} Ast;

Ast ast_from_tokens(Token* token_start);

// ------

bool ast_can_interpret_as_allocator(Token** tokens);

bool ast_can_interpret_as_type(Token** tokens);

bool ast_can_interpret_as_type_with_allocator(Token** tokens);

bool ast_interpret_as_function_declaration(Token* token);

bool ast_interpret_as_variable_declaration(Token* token);

bool ast_interpret_ast_assignment(Token* token);

u32 ast_token_precedence(TokenType type);
// ------

Ast ast_general_parse(Token** tokens);

Ast ast_general_id_parse(Token** tokens);

Ast ast_function_declaration_parse(Token** tokens);

Ast ast_type_parse(Token** tokens);

Ast ast_parameter_parse(Token** tokens);

Ast ast_parameter_list_parse(Token** tokens);

Ast ast_body_parse(Token** tokens);

Ast ast_variable_declaration_parse(Token** tokens);

Ast ast_assignment_parse(Token** tokens, Ast* assignee);

Ast ast_return_parse(Token** tokens);

Ast ast_expression_parse(Token** tokens, TokenType* delimiters, u32 num_delimiters);

Ast ast_expression_value_parse(Token** tokens, TokenType* delimiters, u32 num_delimiters);

Ast ast_int_parse(Token** tokens);

Ast ast_float_parse(Token** tokens);

Ast ast_variable_parse(Token** tokens);

Ast ast_program_parse(Token** tokens);

Ast ast_value_access_parse(Token** tokens, Ast* lhs);

Ast ast_alloc_parse(Token** tokens);

Ast ast_function_call_parse(Token** tokens);

Ast ast_function_call_parameter_parse(Token** tokens);

Ast ast_allocator_parse(Token** tokens);

Ast ast_get_parse(Token** tokens, Ast* lhs);

Ast ast_ptr_parse(Token** tokens, Ast* lhs);

Ast ast_struct_parse(Token** tokens);

Ast ast_struct_body_parse(Token** tokens);

Ast ast_struct_field_parse(Token** tokens);
