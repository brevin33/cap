#include "cap.h"

void log_errorv(const char* message, va_list args) {
    if (!cap_context.log) return;
    cap_context.error_count++;
    printf("\x1b[31m");
    printf("Error: ");
    vprintf(message, args);
    printf("\n");
    printf("\x1b[0m");
}

void log_error(const char* message, ...) {
    va_list args;
    va_start(args, message);
    log_errorv(message, args);
    va_end(args);
}

void log_error_substring(String substring, const char* message, ...) {
    if (!cap_context.log) return;
    cap_context.error_count++;
    printf("\x1b[31m");
    va_list args;
    va_start(args, message);

    printf("Error ");
    _log_chunk(message, args, NULL, substring);

    va_end(args);
    printf("\x1b[0m");
}

void log_error_token(Cap_File* file, Token token, const char* message, ...) {
    if (!cap_context.log) return;
    cap_context.error_count++;
    printf("\x1b[31m");
    if (cap_context.error_count > 16) mabort(str("Too many errors"));

    String substring = token.content;

    va_list args;
    va_start(args, message);
    printf("Error at token ");
    String token_str = token_token_to_string(token);
    printf("\"%.*s\" ", str_info(token_str));
    _log_chunk(message, args, file, substring);
    va_end(args);

    printf("\x1b[0m");
}

void log_error_ast(Ast* ast, const char* message, ...) {
    if (ast == NULL) {
        va_list args;
        va_start(args, message);
        log_errorv(message, args);
        va_end(args);
        return;
    }
    if (!cap_context.log) return;
    cap_context.error_count++;
    printf("\x1b[31m");

    String substring = ast_get_substring(ast);

    va_list args;
    va_start(args, message);
    printf("Error at ast ");
    String ast_str = ast_to_string_short(ast);
    printf("\"%.*s\" ", str_info(ast_str));
    _log_chunk(message, args, ast->file, substring);
    va_end(args);

    printf("\x1b[0m");
}

void log_warningv(const char* message, va_list args) {
    if (!cap_context.log) return;
    printf("\x1b[33m");
    printf("Warning: ");
    vprintf(message, args);
    printf("\n");
    printf("\x1b[0m");
}

void log_warning(const char* message, ...) {
    va_list args;
    va_start(args, message);
    log_warningv(message, args);
    va_end(args);
}

void log_warning_substring(String substring, const char* message, ...) {
    if (!cap_context.log) return;
    printf("\x1b[33m");
    va_list args;
    va_start(args, message);

    printf("Warning ");
    _log_chunk(message, args, NULL, substring);

    va_end(args);
    printf("\x1b[0m");
}

void log_warning_token(Cap_File* file, Token token, const char* message, ...) {
    if (!cap_context.log) return;
    printf("\x1b[33m");
    String substring = token.content;

    va_list args;
    va_start(args, message);
    printf("Warning at token ");
    String token_str = token_token_to_string(token);
    printf("\"%.*s\" ", str_info(token_str));
    _log_chunk(message, args, file, substring);
    va_end(args);

    printf("\x1b[0m");
}

void log_warning_ast(Ast* ast, const char* message, ...) {
    if (ast == NULL) {
        va_list args;
        va_start(args, message);
        log_warningv(message, args);
        va_end(args);
        return;
    }
    if (!cap_context.log) return;
    printf("\x1b[33m");
    String substring = ast_get_substring(ast);

    va_list args;
    va_start(args, message);
    printf("Warning at ast ");
    String ast_str = ast_to_string_short(ast);
    printf("\"%.*s\" ", str_info(ast_str));
    _log_chunk(message, args, ast->file, substring);
    va_end(args);

    printf("\x1b[0m");
}

void log_infov(const char* message, va_list args) {
    if (!cap_context.log) return;
    printf("\x1b[34m");
    printf("Info: ");
    vprintf(message, args);
    printf("\n");
    printf("\x1b[0m");
}

void log_info(const char* message, ...) {
    va_list args;
    va_start(args, message);
    log_infov(message, args);
    va_end(args);
}

void log_info_substring(String substring, const char* message, ...) {
    if (!cap_context.log) return;
    printf("\x1b[34m");
    va_list args;
    va_start(args, message);

    printf("Info ");
    _log_chunk(message, args, NULL, substring);

    va_end(args);
    printf("\x1b[0m");
}

void log_info_token(Cap_File* file, Token token, const char* message, ...) {
    if (!cap_context.log) return;
    printf("\x1b[34m");
    String substring = token.content;

    va_list args;
    va_start(args, message);
    printf("Info at token ");
    String token_str = token_token_to_string(token);
    printf("\"%.*s\" ", str_info(token_str));
    _log_chunk(message, args, file, substring);
    va_end(args);

    printf("\x1b[0m");
}

void log_info_ast(Ast* ast, const char* message, ...) {
    if (ast == NULL) {
        va_list args;
        va_start(args, message);
        log_infov(message, args);
        va_end(args);
        return;
    }
    if (!cap_context.log) return;
    printf("\x1b[34m");
    String substring = ast_get_substring(ast);

    va_list args;
    va_start(args, message);
    printf("Info at ast ");
    String ast_str = ast_to_string_short(ast);
    printf("\"%.*s\" ", str_info(ast_str));
    _log_chunk(message, args, ast->file, substring);
    va_end(args);

    printf("\x1b[0m");
}

void log_successv(const char* message, va_list args) {
    if (!cap_context.log) return;
    printf("\x1b[32m");
    vprintf(message, args);
    printf("\n");
    printf("\x1b[0m");
}

void log_success(const char* message, ...) {
    va_list args;
    va_start(args, message);
    log_successv(message, args);
    va_end(args);
}

void log_success_substring(String substring, const char* message, ...) {
    if (!cap_context.log) return;
    printf("\x1b[32m");
    va_list args;
    va_start(args, message);

    printf("Success ");
    _log_chunk(message, args, NULL, substring);

    va_end(args);
    printf("\x1b[0m");
}

void log_success_token(Cap_File* file, Token token, const char* message, ...) {
    if (!cap_context.log) return;
    printf("\x1b[32m");
    String substring = token.content;

    va_list args;
    va_start(args, message);
    printf("Success at token ");
    String token_str = token_token_to_string(token);
    printf("\"%.*s\" ", str_info(token_str));
    _log_chunk(message, args, file, substring);
    va_end(args);

    printf("\x1b[0m");
}

void log_success_ast(Ast* ast, const char* message, ...) {
    if (ast == NULL) {
        va_list args;
        va_start(args, message);
        log_successv(message, args);
        va_end(args);
        return;
    }
    if (!cap_context.log) return;
    printf("\x1b[32m");
    String substring = ast_get_substring(ast);

    va_list args;
    va_start(args, message);
    printf("Success at ast ");
    String ast_str = ast_to_string_short(ast);
    printf("\"%.*s\" ", str_info(ast_str));
    _log_chunk(message, args, ast->file, substring);
    va_end(args);

    printf("\x1b[0m");
}

void _log_chunk(const char* message, va_list args, Cap_File* file, String substring) {
    u64 line = 1;
    String file_content = file->content;
    char* walk = file_content.data;
    while (walk != substring.data) {
        if (*walk == '\n') line++;
        walk++;
    }
    u64 line_end = line;
    while (substring.data + substring.length != walk) {
        if (*walk == '\n') line_end++;
        walk++;
    }
    if (substring.length != 0 && *(walk - 1) == '\n') line_end--;

    if (line_end == line) {
        printf("on line %llu in file %.*s: ", line, str_info(file->path));
    } else {
        printf("on lines %llu-%llu in file %.*s: ", line, line_end, str_info(file->path));
    }
    vprintf(message, args);
    printf("\x1b[0m");
    printf("\n");

    walk = substring.data - 1;
    while (walk >= file_content.data && *walk != '\n') walk--;
    if (walk != file_content.data) walk++;

    while (walk < substring.data + substring.length) {
        printf("%6llu | ", line);
        char* walk_start = walk;
        while (*walk != '\n' && walk < file_content.data + file_content.length) {
            if (*walk == '\r')
                ;
            else if (*walk == '\t') printf("    ");
            else printf("%c", *walk);
            walk++;
        }
        bool endline_in_substring = walk >= substring.data && walk < substring.data + substring.length;
        if (endline_in_substring) printf("\\n");
        printf("\n");
        printf("%6llu | ", line);
        walk = walk_start;
        while (*walk != '\n' && walk < file_content.data + file_content.length) {
            bool is_in_substring = walk >= substring.data && walk < substring.data + substring.length;
            if (*walk == '\r')
                ;
            else if (*walk == '\t' && is_in_substring) printf("^^^^");
            else if (*walk == '\t') printf("    ");
            else if (is_in_substring) printf("^");
            else printf(" ");
            walk++;
        }
        if (endline_in_substring) printf("^^");
        printf("\n");
        walk++;
    }
}

void rainbow_printf(const char* format, ...) {
    const char* rainbow_colors[] = {
        "\033[31m",  // Red
        "\033[33m",  // Yellow
        "\033[32m",  // Green
        "\033[36m",  // Cyan
        "\033[34m",  // Blue
        "\033[35m",  // Magenta
    };
    char buffer[4096];
    va_list args;

    // Format the string with variable arguments
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Print each character with a different color
    int num_colors = 6;
    int color_index = 0;
    for (int i = 0; buffer[i] != '\0'; i++) {
        // Skip color cycling for whitespace
        if (buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t') {
            printf("%c", buffer[i]);
        } else {
            printf("%s%c", rainbow_colors[color_index], buffer[i]);
            color_index = (color_index + 1) % num_colors;
        }
    }

    // Reset color at the end
    printf("\x1b[0m");
    fflush(stdout);
}
