#include "cap.h"

void log_error_ast(Ast *ast, const char *format, ...) {
    Token start_token = *ast->token_start;
    Token end_token = *(ast->token_start + ast->num_tokens - 1);
    va_list args;
    va_start(args, format);
    log_error_va_list(start_token.file_index, start_token.start_char, end_token.end_char, format, 0, args);
    va_end(args);
}

void log_error_token(Token *token, const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_error_va_list(token->file_index, token->start_char, token->end_char, format, 0, args);
    va_end(args);
}

void log_error(u32 file_id, u32 start_char, u32 end_char, const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_error_va_list(file_id, start_char, end_char, format, 0, args);
    va_end(args);
}

void log_info_ast(Ast *ast, const char *format, ...) {
    Token start_token = *ast->token_start;
    Token end_token = *(ast->token_start + ast->num_tokens - 1);
    va_list args;
    va_start(args, format);
    log_error_va_list(start_token.file_index, start_token.start_char, end_token.end_char, format, 1, args);
    va_end(args);
}

void log_info_token(Token *token, const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_error_va_list(token->file_index, token->start_char, token->end_char, format, 1, args);
    va_end(args);
}

void log_info(Ast *ast, const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_error_va_list(ast->token_start->file_index, ast->token_start->start_char, ast->token_start->end_char, format, 1, args);
    va_end(args);
}

void log_warning_ast(Ast *ast, const char *format, ...) {
    Token start_token = *ast->token_start;
    Token end_token = *(ast->token_start + ast->num_tokens - 1);
    va_list args;
    va_start(args, format);
    log_error_va_list(start_token.file_index, start_token.start_char, end_token.end_char, format, 1, args);
    va_end(args);
}

void log_warning_token(Token *token, const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_error_va_list(token->file_index, token->start_char, token->end_char, format, 2, args);
    va_end(args);
}

void log_warning(u32 file_id, u32 start_char, u32 end_char, const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_error_va_list(file_id, start_char, end_char, format, 2, args);
    va_end(args);
}

void log_error_va_list(u32 file_id, u32 start_char, u32 end_char, const char *format, u32 type, va_list args) {
    if (!cap_context.log_errors) return;
    if (type == 0) cap_context.error_count++;
    File *file = *File_Ptr_List_get(&cap_context.files, file_id);

    u32 line_start = file_get_line_of_index(file, start_char);
    u32 line_end = file_get_line_of_index(file, end_char);
    if (line_start == line_end) {
        if (type == 0)
            red_printf("Error in file %s on line %d: ", file->path, line_start);
        else if (type == 1)
            yellow_printf("Warning in file %s on line %d: ", file->path, line_start);
        else
            blue_printf("Info in file %s on line %d: ", file->path, line_start);
    } else {
        if (type == 0)
            red_printf("Error in file %s on lines %d-%d: ", file->path, line_start, line_end);
        else if (type == 1)
            yellow_printf("Warning in file %s on lines %d-%d: ", file->path, line_start, line_end);
        else
            blue_printf("Info in file %s on lines %d-%d: ", file->path, line_start, line_end);
    }

    if (type == 0)
        printf("\x1b[31m");
    else if (type == 1)
        printf("\x1b[33m");
    else
        printf("\x1b[34m");
    vprintf(format, args);
    printf("\x1b[0m");
    printf("\n");

    for (u32 line = line_start; line <= line_end; line++) {
        printf("%5d |", line);
        u32 index = file_get_front_of_line(file, line);
        char *front_of_line = &file->contents[index];
        print_until_delimiter(front_of_line, '\n');
        printf("\n");
        printf("%5d |", line);
        while (file->contents[index] != '\n' && file->contents[index] != '\0') {
            char c = file->contents[index];
            if (c == '\r') {
                printf("\r");
                index++;
                continue;
            }
            if (c == '\t') {
                printf("\t");
                index++;
                continue;
            }
            bool in_range = index >= start_char && index < end_char;
            if (in_range) {
                printf("^");
            } else {
                printf(" ");
            }
            index++;
        }
        bool in_range = index >= start_char && index < end_char;
        if (in_range) {
            printf("^");
        }
        printf("\n");
    }
}
