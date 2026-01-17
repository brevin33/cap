#pragma once

#include "cap/arena.h"
#include "cap/base.h"
#include "cap/string.h"

typedef struct Ast Ast;

typedef struct Type Type;
typedef struct Type_Pointer Type_Pointer;
typedef struct Type_Reference Type_Reference;
typedef struct Type_Int Type_Int;
typedef struct Type_Float Type_Float;
typedef struct Type_Bool Type_Bool;
typedef struct Type_Uint Type_Uint;
typedef struct Type_Function Type_Function;

typedef struct Program Program;
typedef struct Function Function;
typedef struct Function_Implementation Function_Implementation;
typedef struct Scope Scope;
typedef struct Variable Variable;

typedef struct Statement Statement;
typedef struct Statement_Expression Statement_Expression;
typedef struct Statement_Assignment Statement_Assignment;
typedef struct Statement_Return Statement_Return;

typedef struct Expression Expression;
typedef struct Expression_Int Expression_Int;
typedef struct Expression_Float Expression_Float;
typedef struct Expression_Variable Expression_Variable;
typedef struct Expression_Variable_Declaration Expression_Variable_Declaration;
typedef struct Expression_Dereference Expression_Dereference;
typedef struct Expression_Cast Expression_Cast;
typedef struct Expression_Reference Expression_Reference;
typedef struct Expression_Function_Call Expression_Function_Call;

typedef struct Allocator Allocator;
typedef struct Allocator_Map Allocator_Map;
typedef struct Allocator_Id_Data Allocator_Id_Data;
typedef struct Function_Implementation_Parameter Function_Implementation_Parameter;
typedef u64 Allocator_Id;

typedef struct Compile_Time_Value Compile_Time_Value;

typedef struct Cap_Interperter Cap_Interperter;
typedef struct Interpreter_Variable_To_Memory Interpreter_Variable_To_Memory;

typedef enum Type_Kind {
    type_invalid = 0,
    type_pointer,
    type_int,
    type_float,
    type_bool,
    type_uint,
    type_reference,
    type_void,
    type_type,
    type_function,
    type_int_literal,
    type_float_literal,
} Type_Kind;

struct Type_Pointer {
    Type* underlying_type;
};

struct Type_Reference {
    Type* underlying_type;
};

struct Type_Int {
    i64 bits;
};

struct Type_Float {
    i64 bits;
};

struct Type_Bool {
    i64 bits;
};

struct Type_Uint {
    i64 bits;
};

struct Type_Function {
    Type* return_types;
    u64 return_types_count;
    Type* parameter_types;
    u64 parameter_types_count;
};

struct Type {
    Type_Kind kind;
    Allocator_Id allocator_id;
    bool is_complete;
    union {
        Type_Pointer pointer;
        Type_Reference reference;
        Type_Int int_;
        Type_Float float_;
        Type_Bool bool_;
        Type_Uint uint;
        Type_Function function;
    };
};

#define NO_ALLOCATOR_ID UINT64_MAX
struct Allocator {
    // TODO: fill this out
    u64 temp;
};

struct Allocator_Map {
    Allocator** allocator;
    u64 allocator_count;
    u64 allocator_capacity;
};

struct Function {
    Ast* ast;

    Type function_type;
    String* parameter_names;

    Function_Implementation** implementations;
    u32 implementations_count;
    u32 implementations_capacity;

    Scope* scope_created_in;
    u64 namespace_id;
};

typedef enum Expression_Kind {
    expression_invalid = 0,
    expression_int,
    expression_float,
    expression_variable,
    expression_variable_declaration,
    expression_dereference,
    expression_cast,
    expression_reference,
    expression_multiply,
    expression_function_call,
} Expression_Kind;

struct Expression_Cast {
    Expression* expr;
};

struct Expression_Int {
    i64 value;
};

struct Expression_Float {
    f64 value;
};

struct Expression_Variable {
    Variable* variable;
};

struct Expression_Variable_Declaration {
    Variable* variable;
    String name;
    Type type;
};

struct Expression_Dereference {
    Expression* expr;
};

struct Expression_Reference {
    Expression* expr;
};

struct Expression_Function_Call {
    Expression* parameters;
    u64 parameter_count;
    Function_Implementation* implementation;
};

// if type kind is type_reference then is value is the underlying type
struct Compile_Time_Value {
    bool has_value;
    union {
        Function* function;
        Type* underlying_type;
        i8 i8_value;
        i16 i16_value;
        i32 i32_value;
        i64 i64_value;
        u8 u8_value;
        u16 u16_value;
        u32 u32_value;
        u64 u64_value;
        f32 f32_value;
        f64 f64_value;
        i64 int_literal_value;
        f64 float_literal_value;
    };
};

struct Expression {
    Expression_Kind kind;
    Type type;
    Ast* ast;
    Compile_Time_Value compile_time_value;
    union {
        Expression_Int int_value;
        Expression_Float float_value;
        Expression_Variable variable;
        Expression_Variable_Declaration variable_declaration;
        Expression_Dereference dereference;
        Expression_Cast cast;
        Expression_Reference reference;
        Expression_Function_Call function_call;
    };
};

typedef enum Statement_Kind {
    statement_invalid = 0,
    statement_expression,
    statement_assignment,
    statement_return,
} Statement_Kind;

struct Statement_Expression {
    Expression expression;
};

struct Statement_Assignment {
    Expression* assignees;
    Expression* values;
    u64 count;
};

struct Statement_Return {
    Expression value;
};

struct Statement {
    Statement_Kind kind;
    Ast* ast;
    union {
        Statement_Expression expression;
        Statement_Assignment assignment;
        Statement_Return return_;
    };
};

struct Scope {
    Variable** variables;
    u32 variables_count;
    u32 variables_capacity;

    Statement* statements;
    u32 statements_count;
    u32 statements_capacity;

    Scope* parent;
};

struct Variable {
    Type type;
    String name;
    Ast* ast;
    u64 namespace;
    Compile_Time_Value compile_time_value;
};

struct Function_Implementation_Parameter {
    Type type;
    Compile_Time_Value compile_time_value;
};

struct Function_Implementation {
    u64 parameter_count;
    Function_Implementation_Parameter* parameters;
    Type return_type;
    Scope body;
};

struct Program {
    String name;
    Function function;
};

struct Interpreter_Variable_To_Memory {
    Variable* variables;
    void* memory_of_variable;
    u64 capacity;
    u64 count;
};

struct Cap_Interperter {
    Arena memory;
    Interpreter_Variable_To_Memory* scopes;
};

Function sem_function_parse(Ast* ast);
Program sem_program_parse(Ast* ast);

Type sem_type_parse(Ast* ast);

void sem_complete_types_in_global_scope();
void sem_complete_variable_type(Variable* variable);

Type sem_void_type();
Type sem_type_type();
Type sem_int_type(i64 bits);
Type sem_uint_type(i64 bits);
Type sem_bool_type(i64 bits);
Type sem_float_type(i64 bits);
Type sem_function_type(Type* return_types, u64 return_types_count, Type* parameter_types, u64 parameter_types_count, Allocator_Id allocator_id);
Type sem_type_reference(Type* underlying_type, Ast* ast_for_error);
Type sem_type_pointer(Type* underlying_type, Ast* ast_for_error);
Type sem_type_int_literal();
Type sem_type_float_literal();
Type sem_type_invalid();

Type sem_type_dereference(Type* type);
Type sem_type_underlying_type(Type* type);
Type sem_type_new_allocator_ids(Type* type);

bool sem_type_is_reference_of(Type* type, Type* underlying_type);
bool sem_type_is_ptr_to(Type* type, Type* underlying_type);

String sem_type_to_string(Type* type);
Function sem_create_function(Type function_type, String* parameter_names, Ast* ast);
Variable* sem_find_variable(String name, String* namespaces, u64 namespaces_count, Ast* ast_for_error);
Variable* sem_add_variable(String name, Type type, Ast* ast, Compile_Time_Value compile_time_value);
Variable** sem_find_functions_with_name_and_namespace(String name, String* namespaces, u64 namespaces_count, u64* out_count);
Expression sem_function_call(Function* function, Expression* parameters, u64 parameter_count, Ast* ast);
Function* sem_find_function(String name, String* namespaces, u64 namespaces_count, Type* types, u64 types_count, Ast* ast);
Expression sem_expression_parse_with_variable_declaration(Ast* ast);
Expression sem_expression_parse(Ast* ast);
Expression sem_expression_nil_biop_parse(Ast* ast);
Expression sem_expression_variable_parse(Ast* ast);
Expression sem_expression_int_parse(Ast* ast);
Expression sem_expression_float_parse(Ast* ast);

Expression sem_reference(Expression* expr);
Expression sem_dereference(Expression* expr);
Expression sem_implicit_cast(Expression* expr, Type* type);
bool sem_can_implicit_cast(Expression* expr, Type* type);
Expression sem_implicit_cast_without_allocator(Expression* expr, Type* type);
Expression sem_cast_without_allocator(Expression* expr, Type* type);
Expression sem_cast(Expression* expr, Type* type);

bool sem_type_equal(Type* type_a, Type* type_b);
bool sem_type_equal_without_allocator(Type* type_a, Type* type_b);
bool sem_type_allocator_equal(Type* type_a, Type* type_b);

Allocator_Id sem_get_new_allocator_id();
void sem_set_id_allocator(Allocator_Id id, Allocator* allocator);
void sem_connect_allocator_ids(Allocator_Id id1, Allocator_Id id2);

void sem_scope_parse_statements(Ast* ast, Scope* scope);
Statement sem_statement_parse(Ast* ast);

bool sem_variable_fits_namespace(Variable* variable, String* namespaces, u64 namespaces_count);

bool sem_assign_expression(Expression* assignee, Expression* value);
