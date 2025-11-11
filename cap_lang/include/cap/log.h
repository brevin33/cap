#pragma once

#include "base.h"
#include "cap/ast.h"
#include "cap/tokens.h"

void log_error(u32 file, u32 start_char, u32 end_char, const char *format, ...);

void log_error_va_list(u32 file, u32 start_char, u32 end_char, const char *format, va_list args);

void log_error_token(Token *token, const char *format, ...);

void log_error_ast(Ast *ast, const char *format, ...);
