#pragma once

#include "project.h"
#include "util/util.h"

typedef struct Log Log;
typedef struct Log_List Log_List;

typedef enum Log_Level {
    log_invalid = 0,
    log_error = 1,
    log_warning = 2,
    log_info = 3,
    log_debug = 4,
} Log_Level;

struct Log {
    Log_Level level;
    utf8 msg;
    utf8 sub_string;
    Token token;
    Ast* ast;
    SSA* ssa;
    Cap_File* file;
};

u32 log_end_location();

void log_clear();

void log_clear_after(u32 location);

void log_output(Log_Level level);

bool log_has_error();

bool log_has_error_after(u32 location);

bool log_has_msg_after(u32 location);

void log_msg(const char* msg, Log_Level level);

void log_utf8(utf8 msg, Log_Level level);

void log_msg_context(const char* msg, Log_Level level, utf8 sub_string, Cap_File* file);

void log_utf8_context(utf8 msg, Log_Level level, utf8 sub_string, Cap_File* file);

void log_msg_token(const char* msg, Log_Level level, Token token, Cap_File* file);

void log_utf8_token(utf8 msg, Log_Level level, Token token, Cap_File* file);

void log_msg_ast(const char* msg, Log_Level level, Ast* ast);

void log_utf8_ast(utf8 msg, Log_Level level, Ast* ast);

void log_msg_ssa(const char* msg, Log_Level level, SSA* ssa);

void log_utf8_ssa(utf8 msg, Log_Level level, SSA* ssa);

void log_append(Log* log);

bool log_has_location(Log* log);
