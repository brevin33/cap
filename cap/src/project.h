#pragma once

#include "ast.h"
#include "token.h"
#include "util/util.h"

typedef struct Cap_File Cap_File;
typedef struct Cap_Folder Cap_Folder;

struct Cap_File {
    utf8 path;
    utf8 contents;
    Tokens tokens;
    Ast ast;
};

struct Cap_Folder {
    utf8 path;

    Cap_File* files;
    u32 file_count;
    u32 file_capacity;
};

utf8 cap_file_get_line(Cap_File* file, u32 line);

Cap_Folder cap_folder_create_from_path(utf8 path);
