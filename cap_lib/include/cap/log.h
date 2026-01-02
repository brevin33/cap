#pragma once

#include "cap/base.h"
#include "cap/token.h"

typedef struct Cap_File Cap_File;
typedef struct Ast Ast;

void log_error(const char* message, ...);
void log_error_token(Cap_File* file, Token token, const char* message, ...);
void log_error_ast(Cap_File* file, Ast* ast, const char* message, ...);

void log_warning(const char* message, ...);
void log_warning_token(Cap_File* file, Token token, const char* message, ...);
void log_warning_ast(Cap_File* file, Ast* ast, const char* message, ...);

void log_info(const char* message, ...);
void log_info_token(Cap_File* file, Token token, const char* message, ...);
void log_info_ast(Cap_File* file, Ast* ast, const char* message, ...);

void log_success(const char* message, ...);
void log_success_token(Cap_File* file, Token token, const char* message, ...);
void log_success_ast(Cap_File* file, Ast* ast, const char* message, ...);

void _log_chunk(const char* message, va_list args, Cap_File* file, String substring);
