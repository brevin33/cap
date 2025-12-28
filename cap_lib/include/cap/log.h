#pragma once

#include "cap/base.h"
#include "cap/token.h"

void log_error(char* message, ...);

void log_error_token(Tokens tokens, Token token, char* message, ...);

void log_warning(char* message, ...);

void log_warning_token(Tokens tokens, Token token, char* message, ...);

void log_info(char* message, ...);

void log_info_token(Tokens tokens, Token token, char* message, ...);

void log_success(char* message, ...);

void log_success_token(Tokens tokens, Token token, char* message, ...);

void log_chunk(char* message, va_list args, String file_content, String substring);
