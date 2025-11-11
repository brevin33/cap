#pragma once
#include "base.h"
#include "cap/ast.h"
#include "cap/lists.h"
#include "cap/tokens.h"

typedef struct Project {
    File_Ptr_List files;
} Project;

typedef struct File {
    char* path;
    char* contents;
    Token* tokens;
    Function_Ptr_List functions;
    Ast ast;
} File;

File* file_create(const char* path);

u32 file_get_line_of_index(File* file, u32 index);

u32 file_get_front_of_line(File* file, u32 line);
