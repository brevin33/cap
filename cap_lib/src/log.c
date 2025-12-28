#include "cap.h"

void log_error(char* message, ...) {
    printf("\x1b[31m");
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\x1b[0m");
}

void log_error_token(Tokens tokens, Token token, char* message, ...) {
    printf("\x1b[31m");

    String file_content = tokens.file_content;
    String substring = token.content;

    va_list args;
    va_start(args, message);
    log_chunk(message, args, file_content, substring);
    va_end(args);

    printf("\x1b[0m");
}

void log_warning(char* message, ...) {
    printf("\x1b[33m");
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\x1b[0m");
}

void log_warning_token(Tokens tokens, Token token, char* message, ...) {
    printf("\x1b[33m");
    String file_content = tokens.file_content;
    String substring = token.content;

    va_list args;
    va_start(args, message);
    log_chunk(message, args, file_content, substring);
    va_end(args);

    printf("\x1b[0m");
}

void log_info(char* message, ...) {
    printf("\x1b[32m");
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\x1b[0m");
}

void log_info_token(Tokens tokens, Token token, char* message, ...) {
    printf("\x1b[32m");
    String file_content = tokens.file_content;
    String substring = token.content;

    va_list args;
    va_start(args, message);
    log_chunk(message, args, file_content, substring);
    va_end(args);

    printf("\x1b[0m");
}

void log_success(char* message, ...) {
    printf("\x1b[32m");
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\x1b[0m");
}

void log_success_token(Tokens tokens, Token token, char* message, ...) {
    printf("\x1b[32m");
    String file_content = tokens.file_content;
    String substring = token.content;

    va_list args;
    va_start(args, message);
    log_chunk(message, args, file_content, substring);
    va_end(args);

    printf("\x1b[0m");
}

void log_chunk(char* message, va_list args, String file_content, String substring) {
    u64 line = 1;
    char* walk = file_content.data;
    while (walk != substring.data) {
        if (*walk == '\n') line++;
        walk++;
    }

    String start_line = {0};
    while (*walk != '\n') walk--;
    walk++;
    start_line.data = walk;
    while (*walk != '\n') walk++;
    start_line.length = walk - start_line.data;
}
