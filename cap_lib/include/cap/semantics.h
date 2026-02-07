#pragma once

#include "cap/arena.h"
#include "cap/base.h"
#include "cap/string.h"

typedef struct Ast Ast;

typedef struct Type Type;
typedef struct Type_Origin Type_Origin;
typedef struct Type_Call_Origin Type_Call_Origin;
typedef struct Type_Pointer Type_Pointer;
typedef struct Type_Reference Type_Reference;
typedef struct Type_Int Type_Int;
typedef struct Type_Float Type_Float;
typedef struct Type_Bool Type_Bool;
typedef struct Type_Uint Type_Uint;
typedef struct Type_Function Type_Function;
typedef struct Type_Multiple_Value Type_Multiple_Value;
typedef struct Type_Struct Type_Struct;

typedef struct Program Program;
typedef struct Function Function;
typedef struct Function_Implementation Function_Implementation;
typedef struct Scope Scope;
typedef struct Variable Variable;

typedef struct Statement Statement;
typedef struct Statement_Expression Statement_Expression;
typedef struct Statement_Assignment Statement_Assignment;
typedef struct Statement_Return Statement_Return;
typedef struct Statement_Assignment_Multiple_Values Statement_Assignment_Multiple_Values;
typedef struct Statement_Function_Declaration Statement_Function_Declaration;

typedef struct Expression Expression;
typedef struct Expression_Int Expression_Int;
typedef struct Expression_Float Expression_Float;
typedef struct Expression_Variable Expression_Variable;
typedef struct Expression_Variable_Declaration Expression_Variable_Declaration;
typedef struct Expression_Dereference Expression_Dereference;
typedef struct Expression_Cast Expression_Cast;
typedef struct Expression_Reference Expression_Reference;
typedef struct Expression_Function_Call Expression_Function_Call;
typedef struct Expression_Multiple_Values_Access Expression_Multiple_Values_Access;
typedef struct Expression_Passthrough Expression_Passthrough;
typedef struct Expression_Struct Expression_Struct;
typedef struct Expression_Struct_Field_Access Expression_Struct_Field_Access;

typedef struct Allocator Allocator;
typedef struct Allocator_Map Allocator_Map;
typedef struct Allocator_Id_Data Allocator_Id_Data;
typedef u64 Allocator_Id;

typedef struct Cap_Interperter Cap_Interperter;
typedef struct Interpreter_Variable_To_Memory Interpreter_Variable_To_Memory;

typedef enum Type_Kind {
    type_invalid = 0,
    type_pointer,
    type_int,
    type_float,
    type_uint,
    type_reference,
    type_void,
    type_type,
    type_function,
    type_int_literal,
    type_float_literal,
    type_multiple_value,
    type_struct,
} Type_Kind;

struct Type_Struct {
    String* field_names;
    Type** field_types;  // double pointer for interperter interop
    u64 field_count;
};

struct Type_Multiple_Value {
    Type* types;
    u64 types_count;
};

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

struct Type_Uint {
    i64 bits;
};

struct Type_Function {
    Type* return_types;
    u64 return_types_count;
    Type* parameter_types;
    u64 parameter_types_count;
};

typedef enum Type_Origin_Kind {
    type_intrisic_origin = 0,
    type_call_origin,
    type_variable_origin,
} Type_Origin_Kind;

struct Type_Call_Origin {
    Function* function;
    Type* parameter_types;
    void** parameter_compile_time_values;
    u64 parameter_count;
};

struct Type_Origin {
    Type_Origin_Kind kind;
    Type_Origin* previous;
    union {
        Variable* variable;
        Type_Call_Origin* call_origin;
    };
};

struct Type {
    Type_Kind kind;
    Type_Origin origin;
    Allocator_Id allocator_id;
    union {
        Type_Pointer pointer;
        Type_Reference reference;
        Type_Int int_;
        Type_Float float_;
        Type_Uint uint;
        Type_Function function;
        Type_Multiple_Value multiple_value;
        Type_Struct struct_;
    };
};

#define NO_ALLOCATOR_ID UINT64_MAX
struct Allocator {
    Variable* variable;
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
    expression_function_call,
    expression_multiple_values_access,
    expression_passthrough,
    expression_struct,
    expression_struct_field_access,
    // expression_alloc,
} Expression_Kind;

// struct Expression_Alloc {
//     Type* type_to_allocate;
//     u64 count;
// };

struct Expression_Struct_Field_Access {
    Expression* struct_value;
    u64 field_index;
};

struct Expression_Multiple_Values_Access {
    Expression* multiple_values_value;
    u64 index;
};

struct Expression_Cast {
    Expression* expr;
};

struct Expression_Passthrough {
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

struct Expression_Struct {
    Expression* field_types;
    String* field_names;
    u64 field_count;
};

struct Expression {
    Expression_Kind kind;
    Type type;
    Ast* ast;
    union {
        Expression_Int int_value;
        Expression_Float float_value;
        Expression_Variable variable;
        Expression_Variable_Declaration variable_declaration;
        Expression_Dereference dereference;
        Expression_Cast cast;
        Expression_Reference reference;
        Expression_Function_Call function_call;
        Expression_Multiple_Values_Access multiple_values_access;
        Expression_Passthrough passthrough;
        Expression_Struct struct_;
        Expression_Struct_Field_Access struct_field_access;
    };
};

typedef enum Statement_Kind {
    statement_invalid = 0,
    statement_expression,
    statement_assignment,
    statement_assignment_multiple_values,
    statement_return,
    statement_function_declaration,
} Statement_Kind;

struct Statement_Expression {
    Expression expression;
};

struct Statement_Assignment {
    Expression* assignees;
    Expression* values;
    u64 assignees_count;
    u64 values_count;
};

struct Statement_Assignment_Multiple_Values {
    Expression* assignees;
    Expression* values;
    Expression* multiple_values_value;
    u64 count;
};

struct Statement_Return {
    Expression* values;
    u64 values_count;
};

struct Statement_Function_Declaration {
    Variable* variable;
};

struct Statement {
    Statement_Kind kind;
    Ast* ast;
    union {
        Statement_Expression expression;
        Statement_Assignment assignment;
        Statement_Assignment_Multiple_Values assignment_multiple_values;
        Statement_Return return_;
        Statement_Function_Declaration function_declaration;
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
    Expression* initial_value;
    bool know_compile_time_value;
    void* compile_time_value;
    Ast* lost_constant_at_ast;  // null mean it is constant
};

struct Function_Implementation {
    Function* function;
    u64 parameter_count;
    Variable** parameters;
    Type* return_types;
    u64 return_types_count;
    bool is_complete;
    Scope body;
};

struct Program {
    String name;
    Function function;
};

Function sem_function_parse(Ast* ast);
Program sem_program_parse(Ast* ast);

Type sem_type_parse(Ast* ast);

Type sem_void_type();
Type sem_type_type();
Type sem_int_type(i64 bits);
Type sem_uint_type(i64 bits);
Type sem_float_type(i64 bits);
Type sem_function_type(Type* return_types, u64 return_types_count, Type* parameter_types, u64 parameter_types_count, Allocator_Id allocator_id);
Type sem_type_reference(Type* underlying_type, Ast* ast_for_error);
Type sem_type_pointer(Type* underlying_type, Ast* ast_for_error);
Type sem_type_struct(String* field_names, u64 field_names_count, Type** field_types, u64 field_types_count);
Type sem_type_int_literal();
Type sem_type_float_literal();
Type sem_type_invalid();
Type sem_type_multiple_value(Type* types, u64 types_count);

Type sem_type_dereference(Type* type);
Type sem_type_underlying_type(Type* type);
Type sem_type_new_allocator_ids(Type* type);

bool sem_type_is_reference_of(Type* type, Type* underlying_type);
bool sem_type_is_ptr_to(Type* type, Type* underlying_type);

String sem_type_to_string(Type* type);
Function sem_create_function(Type function_type, String* parameter_names, Ast* ast);
Variable* sem_find_variable(String name, String* namespaces, u64 namespaces_count, Ast* ast_for_error);
Variable* sem_add_variable(String name, Type type, Ast* ast);
Variable** sem_find_functions_with_name_and_namespace(String name, String* namespaces, u64 namespaces_count, u64* out_count);
Expression sem_function_call(Function* function, Expression* parameters, u64 parameter_count, Ast* ast);
Expression sem_multiple_values_access(Expression* multiple_values_value, u64 index, Ast* ast);
Function* sem_find_function(String name, String* namespaces, u64 namespaces_count, Type* types, u64 types_count, Ast* ast);
Expression sem_expression_parse_with_variable_declaration(Ast* ast);
Expression sem_expression_variable_parse(Ast* ast);
Expression sem_expression_parse(Ast* ast);
Expression sem_expression_nil_biop_parse(Ast* ast);
Expression sem_expression_int_parse(Ast* ast);
Expression sem_expression_float_parse(Ast* ast);
Expression sem_expression_struct_parse(Ast* ast);

Expression sem_reference(Expression* expr);
Expression sem_dereference(Expression* expr);
Expression sem_implicit_cast(Expression* expr, Type* type);
bool sem_can_implicit_cast(Expression* expr, Type* type);
Expression sem_implicit_cast_without_allocator(Expression* expr, Type* type);
Expression sem_cast_without_allocator(Expression* expr, Type* type);
Expression sem_cast(Expression* expr, Type* type);
Expression sem_passthrough(Expression* expr);
Expression sem_get_value_if(Expression* expr);

bool sem_type_equal(Type* type_a, Type* type_b, bool check_allocator, bool check_origin);

Allocator_Id sem_get_new_allocator_id();
void sem_set_id_allocator(Allocator_Id id, Allocator* allocator);
void sem_connect_allocator_ids(Allocator_Id id1, Allocator_Id id2);
Allocator* sem_get_allocator(Allocator_Id id);

void sem_scope_parse_statements(Ast* ast, Scope* scope);
Statement sem_statement_parse(Ast* ast);
Statement sem_statement_return(Ast* ast);
Statement sem_statement_assignment(Ast* ast);
Statement sem_statement_expression(Ast* ast);
Statement sem_statement_function_declaration(Ast* ast);

void sem_set_variable_compile_time_value(Variable* var, void* value);

Function_Implementation* sem_prototype_implementation(Function* function, Expression* parameters, u64 parameter_count);
void sem_complete_implementation(Function_Implementation* implementation);
void sem_complete_expression(Expression* expression);

void* sem_evaluate_expression(Expression* expression);
bool sem_comile_time_value_is_equal(Type* type, void* value_a, void* value_b);

bool sem_variable_fits_namespace(Variable* variable, String* namespaces, u64 namespaces_count);

bool sem_assign_expression(Expression* assignee, Expression* value, Ast* ast);
