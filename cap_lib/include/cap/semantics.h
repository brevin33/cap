#pragma once

#include "cap/base.h"
#include "cap/string.h"

typedef struct Type Type;
typedef struct Type_Pointer Type_Pointer;
typedef struct Type_Int Type_Int;
typedef struct Type_Float Type_Float;
typedef struct Type_Bool Type_Bool;
typedef struct Type_Uint Type_Uint;
typedef struct Program Program;
typedef struct Function_Parameter Function_Parameter;
typedef struct Function Function;
typedef struct Function_Implementation Function_Implementation;
typedef struct Scope Scope;
typedef struct Variable Variable;
typedef struct Statement Statement;
typedef struct Expression Expression;
typedef struct Allocator Allocator;
typedef struct Allocator_Map Allocator_Map;
typedef struct Allocator_Id_Data Allocator_Id_Data;
typedef u64 Allocator_Id;

typedef enum Type_Kind {
    type_invalid = 0,
} Type_Kind;

struct Type_Pointer {
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

struct Type {
    Type_Kind kind;
    Allocator_Id allocator_id;
    union {
        Type_Pointer pointer;
        Type_Int int_;
        Type_Float float_;
        Type_Bool bool_;
        Type_Uint uint;
    };
};

struct Allocator {};

struct Allocator_Id_Data {
    u64 allocator_index;
    Allocator_Id* connected_ids;
    u64 connected_ids_count;
    u64 connected_ids_capacity;
};

struct Allocator_Map {
    Allocator* allocator;
    u64 allocator_count;
    u64 allocator_capacity;

    Allocator_Id_Data* data;
    u64 data_count;
    u64 data_capacity;
};

typedef enum Statement_Kind {
    statement_invalid = 0,
} Statement_Kind;

struct Statement {
    Statement_Kind kind;
    union {};
};

typedef enum Expression_Kind {
    expression_invalid = 0,
} Expression_Kind;

struct Expression {
    Expression_Kind kind;
    Type type;
    union {};
};

struct Variable {
    Type type;
    String name;
};

struct Scope {
    Variable* variables;
    u32 variables_count;
    u32 variables_capacity;

    Statement* statements;
    u32 statements_count;
    u32 statements_capacity;

    Scope* parent;
};

struct Function_Parameter {
    Type type;
    String name;
};

struct Function {
    String name;
    Type* return_types;

    Function_Parameter* parameters;
    u32 return_types_count;
    u32 parameters_count;

    Function_Implementation* implementation;
    u32 implementation_count;
    u32 implementation_capacity;
};

struct Function_Implementation {
    Scope* body;
};

struct Program {
    String name;
    Function function;
};
