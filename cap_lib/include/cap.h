#pragma once
#include "cap/arena.h"
#include "cap/ast.h"
#include "cap/base.h"
#include "cap/filesystem.h"
#include "cap/log.h"
#include "cap/semantics.h"
#include "cap/string.h"
#include "cap/token.h"

typedef struct Cap_Context Cap_Context;
typedef struct Cap_File Cap_File;
typedef struct Cap_Folder Cap_Folder;
typedef struct Cap_Project Cap_Project;

struct Cap_Context {
    Arena global_arena;
    Arena* active_arena;
    bool log;
    u64 error_count;

    Allocator_Map allocator_map;
    Scope global_scope;

    Variable** visited_in_typing;
    u32 visited_in_typing_count;
    u32 visited_in_typing_capacity;

    Cap_Folder** folders;
    u64 folders_count;
    u64 folders_capacity;

    Scope* scope;
    u64 namespace_we_are_in;

    Function_Implementation* function_being_built;
};

struct Cap_File {
    String path;
    String content;
    Tokens tokens;
    Ast ast;
};

struct Cap_Folder {
    String path;

    Cap_File* files;
    u64 files_count;

    Program* programs;
    u64 programs_count;

    Cap_Folder** folders;
    String* folder_namespace_aliases;
    u64 folders_count;

    u64 namespace_id;
};

struct Cap_Project {
    Cap_Folder* base_folder;
};

extern Cap_Context cap_context;

void cap_init();

void* cap_alloc(u64 size);

Cap_File cap_create_file(String path);

Cap_Folder* cap_create_folder(String path, Cap_Folder** visited_folders, u64 visited_folders_count, u64 visited_folders_capacity);

Cap_Project cap_create_project(String path);
