#pragma once

#include "cap/ast.h"

typedef enum Type_Kind {
    type_invalid = 0,
    type_int,
    type_float,
    type_uint,
    type_const_int,
    type_const_float,
    type_ptr,
    type_ref,
    type_void,
    type_type,
    type_struct,
} Type_Kind;

typedef struct Type_Base Type_Base;
typedef struct Type {
    Type_Base* base;
    u32* ptr_allocator_ids;
    u32 ptr_count;
    u32 base_allocator_id;
    u32 ref_allocator_id;
    bool is_ref;
} Type;

typedef struct Struct_Field Struct_Field;
typedef struct Struct_Field_Allocator_Info {
    bool is_field;
    union {
        Struct_Field* field;
        Variable* global;
    };
} Struct_Field_Allocator_Info;

typedef struct Struct_Field {
    char* name;
    Type type;
    Struct_Field_Allocator_Info base_allocator_info;
    Struct_Field_Allocator_Info_List ptr_allocator_infos;
} Struct_Field;

typedef struct Type_Base_Struct {
    Struct_Field_List fields;
} Type_Base_Struct;

typedef struct Type_Base {
    Ast* ast;
    Type_Kind kind;
    char* name;
    union {
        u32 number_bit_size;
        Type_Base_Struct struct_;
    };
} Type_Base;

typedef struct Function_Parameter {
    Ast* ast;
    Type type;
    char* name;
} Function_Parameter;

typedef struct Variable {
    Type type;
    char* name;
    Ast* ast;
} Variable;

typedef enum Expression_Kind {
    expression_invalid = 0,
    expression_int,
    expression_float,
    expression_variable,
    expression_cast,
    expression_ptr,
    expression_get,
    expression_alloc,
    expression_function_call,
    expression_type,
    expression_type_size,
    expression_type_align,
    expression_struct_access,
} Expression_Kind;

typedef struct Expression_Struct_Access {
    Expression* expression;
    u32 field_index;
} Expression_Struct_Access;

typedef struct Expression_Function_Call {
    Expression_List parameters;
    Templated_Function* templated_function;
} Expression_Function_Call;

typedef struct Expression_Type {
    Type type;
} Expression_Type;

typedef struct Expression_Alloc {
    Expression* type_or_count;
    Expression* count_or_allignment;
    bool is_type_alloc;
} Expression_Alloc;

typedef struct Expression_Type_Size {
    Type type;
} Expression_Type_Size;

typedef struct Expression_Type_Align {
    Type type;
} Expression_Type_Align;

typedef struct Expression_Ptr {
    Expression* expression;
} Expression_Ptr;

typedef struct Expression_Get {
    Expression* expression;
} Expression_Get;

typedef struct Expression_Cast {
    Expression* expression;
} Expression_Cast;

typedef struct Expression_Int {
    char* value;
} Expression_Int;

typedef struct Expression_Float {
    double value;
} Expression_Float;

typedef struct Expression_Variable {
    Variable* variable;
} Expression_Variable;

typedef struct Expression {
    Ast* ast;
    Type type;
    Expression_Kind kind;
    union {
        Expression_Int int_;
        Expression_Float float_;
        Expression_Variable variable;
        Expression_Cast cast;
        Expression_Ptr ptr;
        Expression_Get get;
        Expression_Alloc alloc;
        Expression_Function_Call function_call;
        Expression_Type expression_type;
        Expression_Type_Size type_size;
        Expression_Type_Align type_align;
        Expression_Struct_Access struct_access;
    };
} Expression;

typedef enum Statement_Type {
    statement_invalid = 0,
    statement_assignment,
    statement_return,
    statement_expression,
    statement_variable_declaration,
} Statement_Type;

typedef struct Statement_Variable_Declaration {
    Variable* variable;
    Statement* assignment;
} Statement_Variable_Declaration;

typedef struct Statement_Assignment {
    Expression assignee;
    Expression value;
} Statement_Assignment;

typedef struct Statement_Return {
    Expression value;
} Statement_Return;

typedef struct Statement_Expression {
    Expression value;
} Statement_Expression;

typedef struct Statement {
    Ast* ast;
    Statement_Type type;
    union {
        Statement_Assignment assignment;
        Statement_Return return_;
        Statement_Expression expression;
        Statement_Variable_Declaration variable_declaration;
    };
} Statement;

typedef struct Scope Scope;
typedef struct Scope {
    Ast* ast;
    Scope* parent;
    Variable_Ptr_List variables;
    Statement_List statements;
} Scope;

typedef struct Allocator {
    Variable* variable;
    Scope* scope;
    bool used_for_alloc_or_free;
    bool is_function_value;
} Allocator;

#define UNSPECIFIED_ALLOCATOR \
    (Allocator) { .variable = NULL, .scope = NULL, .used_for_alloc_or_free = false, .is_function_value = false }

#define STACK_ALLOCATOR \
    (Allocator) { .variable = (Variable*)1, .scope = NULL, .used_for_alloc_or_free = false, .is_function_value = false }

// #define LOOSE_ALLOCATOR \
//     (Allocator) { .variable = (Variable*)2, .used_for_alloc_or_free = false, .is_function_value = false }

#define INVALID_ALLOCATOR \
    (Allocator) { .variable = (Variable*)2, .used_for_alloc_or_free = false, .is_function_value = false }

typedef struct Allocator_Connection_Map {
    Allocator_List allocators;
    u32_List_List allocator_id_connections;
} Allocator_Connection_Map;

typedef struct Templated_Function {
    Function* function;
    Type return_type;
    Scope body;
    Scope parameter_scope;
    u32 allocator_id_counter;
    Allocator_Connection_Map allocator_connection_map;
} Templated_Function;

typedef struct Function {
    Ast* ast;
    Type return_type;
    char* name;
    Function_Parameter_List parameters;
    Templated_Function_Ptr_List templated_functions;
    bool is_prototype;
    bool is_template_function;
    bool is_program;
} Function;

typedef struct Program {
    Ast* ast;
    char* name;
    Function body;
} Program;

Program* sem_program_parse(Ast* ast);

Function* sem_function_prototype(Ast* ast);

void sem_templated_function_implement(Templated_Function* function, Ast* scope_ast);

// bool sem_add_type_allocator_id_connection(Templated_Function* templated_function, Type* type1, Type* type2);

Type sem_type_make_allocator_unspecified(Type* type, u32* allocator_id_counter);

void sem_mark_type_as_function_value(Templated_Function* templated_function, Type* type, bool is_return_value);

bool sem_add_allocator_id_connection(Allocator_Connection_Map* map, u32 allocator_id1, u32 allocator_id2, Ast* ast);

void sem_set_allocator(Allocator_Connection_Map* map, u32 allocator_id, Allocator* allocator);

Allocator* sem_allocator_get(Allocator_Connection_Map* map, u32 allocator_id);

bool sem_allocator_are_the_same(Allocator* allocator1, Allocator* allocator2);

bool sem_allocator_are_exactly_the_same(Allocator* allocator1, Allocator* allocator2);

Allocator sem_allocator_parse(Ast* ast, Scope* scope);

Struct_Field sem_struct_field_parse(Ast* ast, Struct_Field_List* other_feilds);

void sem_add_struct(Ast* ast);

bool sem_type_can_be_used_as_allocator(Type* type);

Type sem_type_parse(Ast* ast, Allocator_Connection_Map* map, Scope* scope, u32* allocator_id_counter);

Type sem_type_get_const_int(u32* allocator_id_counter);

Type sem_type_get_const_float(u32* allocator_id_counter);

Type sem_type_get_void(u32* allocator_id_counter);

Type sem_type_get_type(u32* allocator_id_counter);

Type sem_get_int_type(u32 bit_size, u32* allocator_id_counter);

Type sem_get_uint_type(u32 bit_size, u32* allocator_id_counter);

Type sem_get_float_type(u32 bit_size, u32* allocator_id_counter);

Type_Base* sem_find_type_base(const char* name);

void sem_default_setup_types();

Type_Base* add_number_type_base(u32 number_bit_size, Type_Kind kind);

Variable* sem_scope_add_variable(Scope* scope, Variable* variable, Templated_Function* templated_function);

Variable* sem_scope_get_variable(Scope* scope, char* name);

void sem_scope_implement(Scope* scope, Templated_Function* templated_function);

Statement sem_statement_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Statement sem_statement_assignment_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Statement sem_statement_return_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Statement sem_statement_expression_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Statement sem_statement_variable_declaration_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Expression sem_expression_alloc_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Expression sem_expression_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Expression sem_expression_variable_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Expression sem_expression_int_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Expression sem_expression_float_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Expression sem_expression_value_access_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Expression sem_expression_ptr_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Expression sem_expression_type_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Expression sem_expression_get_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Expression sem_expression_cast(Expression* expression, Type* type, Templated_Function* templated_function);

Expression sem_expression_dereference(Expression* expression, Templated_Function* templated_function);

Expression sem_expression_function_call_parse(Ast* ast, Scope* scope, Templated_Function* templated_function);

Expression sem_expression_type_size_parse(Ast* ast, Scope* scope, Templated_Function* templated_function, Expression* type);

Expression sem_expression_type_align_parse(Ast* ast, Scope* scope, Templated_Function* templated_function, Expression* type);

Expression sem_function_call(char* name, Expression_List* parameters, Ast* ast, Templated_Function* calling_templated_function);

Expression sem_expression_struct_access_parse(Ast* ast, Scope* scope, Templated_Function* templated_function, Expression* struct_expr);

bool sem_can_implicitly_cast(Type* from_type, Type* to_type);

Expression sem_expression_implicit_cast(Expression* expression, Type* type, Templated_Function* templated_function);

char* sem_type_name(Type* type);

Type sem_type_copy_allocator_id(Type* to_type, Type* from_type);

Type sem_copy_type(Type* type);

Type sem_dereference_type(Type* type);

Type sem_underlying_type(Type* type);

Type sem_ptr_of_ref(Type* type);

Type sem_pointer_of_type(Type* type, u32* allocator_id_counter);

char* sem_type_name(Type* type);

bool sem_type_is_equal_without_allocator_id(Type* type1, Type* type2);

bool sem_base_allocator_matters(Type* type);

Type_Kind sem_get_type_kind(Type* type);

Allocator_Connection_Map sem_copy_allocator_connection_map(Allocator_Connection_Map* map);
