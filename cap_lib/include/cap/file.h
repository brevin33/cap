#pragma once

#include "cap/base.h"
#include "cap/string.h"
#include "cap/token.h"

typedef struct Cap_File Cap_File;

struct Cap_File {
    String path;
    String content;
    Tokens tokens;
    Ast ast;
};

Cap_File file_create_cap_file(String path);
