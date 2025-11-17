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
} Ast_Kind;

typedef struct Ast Ast;

typedef struct Ast_Type {
    char* name;
} Ast_Type;

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
    };
} Ast;

Ast ast_from_tokens(Token* token_start);

// ------

bool ast_can_interpret_as_type(Token** tokens);

bool ast_interpret_as_function_declaration(Token* token);

bool ast_interpret_as_variable_declaration(Token* token);

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

Ast ast_expression_value_parse(Token** tokens);

Ast ast_int_parse(Token** tokens);

Ast ast_float_parse(Token** tokens);

Ast ast_variable_parse(Token** tokens);

Ast ast_program_parse(Token** tokens);
