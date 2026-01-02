#pragma once

#include "cap/string.h"
#include "cap/token.h"

typedef struct Ast Ast;
typedef struct Ast_Type Ast_Type;
typedef struct Ast_Return Ast_Return;
typedef struct Ast_Int Ast_Int;
typedef struct Ast_Float Ast_Float;
typedef struct Ast_Function_Declaration Ast_Function_Declaration;
typedef struct Ast_Function_Declaration_Parameter Ast_Function_Declaration_Parameter;
typedef struct Ast_Scope Ast_Scope;
typedef struct Ast_Assignment Ast_Assignment;
typedef struct Ast_Variable_Declaration Ast_Variable_Declaration;
typedef struct Ast_Program Ast_Program;
typedef struct Ast_Top_Level Ast_Top_Level;
typedef struct Ast_Function_Scope Ast_Function_Scope;
typedef struct Ast_Variable Ast_Variable;
typedef struct Ast_Dereference Ast_Dereference;
typedef struct Ast_Reference Ast_Reference;
typedef struct Ast_Biop Ast_Biop;

typedef enum Ast_Kind {
    ast_invalid = 0,
    ast_return,
    ast_int,
    ast_float,
    ast_function_declaration,
    ast_function_declaration_parameter,
    ast_scope,
    ast_assignment,
    ast_variable_declaration,
    ast_top_level,
    ast_program,
    ast_function_scope,
    ast_variable,
    ast_dereference,
    ast_reference,
    ast_add,
    ast_subtract,
    ast_multiply,
    ast_divide,
    ast_modulo,
    ast_greater,
    ast_less,
    ast_greater_equal,
    ast_less_equal,
    ast_logical_and,
    ast_bitwise_and,
    ast_logical_or,
    ast_bitwise_or,
    ast_shift_left,
    ast_shift_right,
} Ast_Kind;

struct Ast_Biop {
    Ast* lhs;
    Ast* rhs;
};

struct Ast_Return {
    Ast* value;
};

struct Ast_Int {
    i64 value;
};

struct Ast_Float {
    f64 value;
};

struct Ast_Dereference {
    Ast* value;
};

struct Ast_Reference {
    Ast* value;
};

struct Ast_Function_Declaration_Parameter {
    Ast* type;
    String name;
};

struct Ast_Scope {
    Ast* statements;
    u64 statements_count;
};

struct Ast_Variable {
    String name;
};

struct Ast_Function_Declaration {
    String name;
    Ast* return_types;
    Ast* parameters;
    u32 return_types_count;
    u32 parameters_count;
    Ast* body;
};

struct Ast_Variable_Declaration {
    Ast* type;
    String name;
};

struct Ast_Assignment {
    Ast* assignees;
    Ast* values;
    u16 assignees_count;
    u16 values_count;
};

struct Ast_Top_Level {
    Ast* programs;
    Ast* functions;
    u32 programs_count;
    u32 functions_count;
};

struct Ast_Program {
    String name;
    Ast* body;
};

struct Ast_Function_Scope {
    Ast* statements;
    u32 count;
};

struct Ast {
    Ast_Kind kind;
    Tokens tokens;
    union {
        Ast_Return return_;
        Ast_Int int_value;
        Ast_Float float_value;
        Ast_Program program;
        Ast_Top_Level top_level;
        Ast_Scope scope;
        Ast_Assignment assignment;
        Ast_Variable_Declaration variable_declaration;
        Ast_Function_Declaration function_declaration;
        Ast_Function_Declaration_Parameter function_declaration_parameters;
        Ast_Function_Scope function_scope;
        Ast_Variable variable;
        Ast_Dereference dereference;
        Ast_Reference reference;
        Ast_Biop biop;
    };
};

#define ast_expect(token, _kind, file)                                                                       \
    do {                                                                                                     \
        Token* _temp_token = &(token);                                                                       \
        Cap_File* _temp_file_ref = (file);                                                                   \
        Token_Kind _temp_token_kind = _kind;                                                                 \
        if (_temp_token->kind != (_temp_token_kind)) {                                                       \
            String token_kind_string = token_token_kind_to_string((_temp_token_kind));                       \
            log_error_token((_temp_file_ref), *_temp_token, "Expected %.*s", str_info((token_kind_string))); \
            return (Ast){0};                                                                                 \
        }                                                                                                    \
    } while (0)

Ast ast_parse_tokens(Tokens tokens, Cap_File* file);

Ast ast_parse_top_level_ast(Tokens tokens, u64* i, Cap_File* file);

Ast ast_parse_program(Tokens tokens, u64* i, Cap_File* file);

Ast ast_parse_top_level_statement(Tokens tokens, u64* i, Cap_File* file);

Ast ast_function_scope_parse(Tokens tokens, u64* i, Cap_File* file);

Ast ast_function_scope_statement_parse(Tokens tokens, u64* i, Cap_File* file);

Ast ast_return_parse(Tokens tokens, u64* i, Cap_File* file);

Ast ast_expression_parse(Tokens tokens, u64* i, Cap_File* file, Token_Kind* delimiter, u64 delimiter_count);
Ast _ast_expression_parse(Tokens tokens, u64* i, Cap_File* file, Token_Kind* delimiter, u64 delimiter_count, u64 precedence);

Ast ast_expression_value_parse(Tokens tokens, u64* i, Cap_File* file);

Ast ast_expression_value_parse_identifier(Tokens tokens, u64* i, Cap_File* file);

Ast ast_expression_dereference_parse(Tokens tokens, u64* i, Cap_File* file, Ast* lhs, Token_Kind* delimiter, u64 delimiter_count);
Ast ast_expression_reference_parse(Tokens tokens, u64* i, Cap_File* file, Ast* lhs, Token_Kind* delimiter, u64 delimiter_count);

String ast_kind_to_string(Ast_Kind kind);

String ast_to_string_short(Ast* ast);

Ast ast_function_scope_statement_identifier_parse(Tokens tokens, u64* i, Cap_File* file);

Ast ast_assignee_parse(Tokens tokens, u64* i, Cap_File* file);
Ast ast_assignment_parse(Tokens tokens, u64* i, Cap_File* file);

Ast ast_type_parse(Tokens tokens, u64* i, Cap_File* file);

Ast ast_variable_parse(Tokens tokens, u64* i, Cap_File* file);

bool ast_parse_as_function_declaration(Tokens tokens, u64* i, Cap_File* file);
Ast ast_function_declaration_parse(Tokens tokens, u64* i, Cap_File* file);
Ast ast_function_declaration_parameter_parse(Tokens tokens, u64* i, Cap_File* file);

String ast_get_substring(Ast* ast);

bool ast_can_parse_as_type(Tokens tokens, u64* i, Cap_File* file);
