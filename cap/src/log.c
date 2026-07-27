#include "cap.h"

u32 log_end_location() {
    return context.logs_count;
}

void log_clear() {
    context.logs_count = 0;
}

void log_clear_after(u32 location) {
    context.logs_count = location;
}

bool log_has_error() {
    for (u32 i = 0; i < context.logs_count; i++) {
        Log* log = context.logs + i;
        if (log->level == log_error) return true;
    }
    return false;
}

bool log_has_msg_after(u32 location) {
    for (u32 i = location; i < context.logs_count; i++) {
        return true;
    }
    return false;
}

bool log_has_error_after(u32 location) {
    for (u32 i = location; i < context.logs_count; i++) {
        Log* log = context.logs + i;
        if (log->level == log_error) return true;
    }
    return false;
}

static void log_get_location_sub_string_highlight(Cap_File* file, utf8 sub_string, u32 line, utf8* msg_buffer_utf8, u32* msg_buffer_capacity) {
    utf8 line_contents = cap_file_get_line(file, line);
    bool show_newline = line_contents.data + line_contents.count <= sub_string.data + sub_string.count;
    if (show_newline) {
        char line_number_buffer[32] = {0};
        snprintf(line_number_buffer, sizeof(line_number_buffer), "%5d", line);
        utf8 line_number_utf8 = {line_number_buffer, strlen(line_number_buffer)};

        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, line_number_utf8);
        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, utf8_str(": "));
        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, line_contents);
        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, utf8_str("\\n\n"));

        char line_number_buffer2[32] = {0};
        snprintf(line_number_buffer2, sizeof(line_number_buffer2), "%5d", line);
        utf8 line_number_utf8_2 = {line_number_buffer2, strlen(line_number_buffer2)};
        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, line_number_utf8_2);
        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, utf8_str(": "));
    } else {
        char line_number_buffer[32] = {0};
        snprintf(line_number_buffer, sizeof(line_number_buffer), "%5d", line);
        utf8 line_number_utf8 = {line_number_buffer, strlen(line_number_buffer)};

        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, line_number_utf8);
        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, utf8_str(": "));
        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, line_contents);
        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, utf8_str("\n"));

        char line_number_buffer2[32] = {0};
        snprintf(line_number_buffer2, sizeof(line_number_buffer2), "%5d", line);
        utf8 line_number_utf8_2 = {line_number_buffer2, strlen(line_number_buffer2)};
        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, line_number_utf8_2);
        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, utf8_str(": "));
    }
    while (line_contents.count != 0) {
        char* c = line_contents.data;
        bool in_sub_string = c >= sub_string.data && c < sub_string.data + sub_string.count;
        u32 unicode = utf8_get(line_contents);
        utf8_next(&line_contents);
        char* next_c = line_contents.data;
        if (unicode == '\t') {
            if (in_sub_string) {
                utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, utf8_str("^^^^"));
            } else {
                utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, utf8_str("    "));
            }
        } else if (unicode == '\r') {
            utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, utf8_str("\r"));
        } else {
            if (in_sub_string) {
                utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, utf8_str("^"));
            } else {
                utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, utf8_str(" "));
            }
        }
    }
    if (show_newline) {
        utf8_append_with_capacity(msg_buffer_utf8, msg_buffer_capacity, utf8_str("^^"));
    }
}

void log_output(Log_Level level) {
    u32 msg_buffer_capacity_smem = 128;
    u32* msg_buffer_capacity = &msg_buffer_capacity_smem;
    char* msg_buffer = alloc(msg_buffer_capacity_smem);
    utf8 msg_buffer_utf8 = {msg_buffer, 0};

    for (u32 i = 0; i < context.logs_count; i++) {
        Log* log = context.logs + i;
        if (log->level > level) continue;

        u32 line_start = 1;
        u32 line_end = 1;
        if (log_has_location(log)) {
            utf8 sub_string = log->sub_string;
            Cap_File* file = log->file;

            // find line start and end
            utf8 line_count_walker = file->contents;
            while (sub_string.data != line_count_walker.data) {
                u32 unicode = utf8_get(line_count_walker);
                utf8_next(&line_count_walker);
                if (unicode == '\n') {
                    line_start++;
                }
            }
            line_end = line_start;
            char* sub_string_end_ptr = sub_string.data + sub_string.count - 1;
            while (sub_string_end_ptr != line_count_walker.data) {
                u32 unicode = utf8_get(line_count_walker);
                utf8_next(&line_count_walker);
                if (unicode == '\n') {
                    line_end++;
                }
            }
        }

        switch (log->level) {
            case log_error: {
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("\033[31mError"));
                break;
            }
            case log_warning: {
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("\033[33mWarning"));
                break;
            }
            case log_info: {
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("\033[36mInfo"));
                break;
            }
            case log_debug: {
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("\033[37mDebug"));
                break;
            }
            case log_invalid: {
                internal_compiler_error();
            }
        }

        if (log_has_location(log)) {
            Cap_File* file = log->file;
            if (line_start == line_end) {
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str(" on line "));

                char line_start_buf[32] = {0};
                snprintf(line_start_buf, sizeof(line_start_buf), "%d", line_start);

                utf8 line_start_utf8 = {line_start_buf, 1};
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, line_start_utf8);

                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str(" in "));
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, file->path);
            } else {
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str(" on line "));

                char line_start_buf[32];
                snprintf(line_start_buf, 32, "%d", line_start);
                utf8 line_start_utf8 = {line_start_buf, strlen(line_start_buf)};
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, line_start_utf8);

                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("-"));

                char line_end_buf[32];
                snprintf(line_end_buf, 32, "%d", line_end);
                utf8 line_end_utf8 = {line_end_buf, strlen(line_end_buf)};
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, line_end_utf8);

                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str(" in "));
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, file->path);
            }
        }

        if (log->token.kind != Token_Kind_Invalid) {
            utf8 token_type_string = token_kind_to_string(log->token.kind);
            utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str(" at "));
            utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, token_type_string);
        }

        if (log->ast != NULL) {
            utf8 ast_type_string = ast_kind_to_string(log->ast->kind);
            utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str(" at "));
            utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, ast_type_string);
        }

        if (log->ssa != NULL) {
            utf8 ssa_type_string = ssa_kind_to_string(log->ssa->kind);
            utf8 ssa_name = ssa_get_ssa_name(log->ssa);

            utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str(" at "));
            utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, ssa_name);
            switch (log->level) {
                case log_error: {
                    utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("\033[31m"));
                    break;
                }
                case log_warning: {
                    utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("\033[33m"));
                    break;
                }
                case log_info: {
                    utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("\033[36m"));
                    break;
                }
                case log_debug: {
                    utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("\033[37m"));
                    break;
                }
                case log_invalid: {
                    internal_compiler_error();
                }
            }
            utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("("));
            utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, ssa_type_string);
            utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str(")"));
        }

        utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str(": "));
        utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, log->msg);
        utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("\033[0m\n"));

        if (log_has_location(log)) {
            Cap_File* file = log->file;
            utf8 sub_string = log->sub_string;

            log_get_location_sub_string_highlight(file, sub_string, line_start, &msg_buffer_utf8, msg_buffer_capacity);

            utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("\n"));

            if (line_start - line_end >= 3) {
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("       . . .\n"));
            }

            if (line_start != line_end) {
                log_get_location_sub_string_highlight(file, sub_string, line_end, &msg_buffer_utf8, msg_buffer_capacity);
                utf8_append_with_capacity(&msg_buffer_utf8, msg_buffer_capacity, utf8_str("\n"));
            }
        }
    }

    // swap tabs for 4 spaces
    msg_buffer = msg_buffer_utf8.data;
    char* msg_buffer2 = alloc(msg_buffer_capacity_smem);
    memcpy(msg_buffer2, msg_buffer, msg_buffer_utf8.count);
    u64 msg_buffer_index = 0;
    for (u64 i = 0; i < msg_buffer_utf8.count; i++) {
        if (msg_buffer2[i] == '\t') {
            ptr_append(msg_buffer, msg_buffer_index, msg_buffer_capacity_smem, ' ');
            ptr_append(msg_buffer, msg_buffer_index, msg_buffer_capacity_smem, ' ');
            ptr_append(msg_buffer, msg_buffer_index, msg_buffer_capacity_smem, ' ');
            ptr_append(msg_buffer, msg_buffer_index, msg_buffer_capacity_smem, ' ');
        } else if (msg_buffer2[i] == '\r') {
            // skip
        } else {
            ptr_append(msg_buffer, msg_buffer_index, msg_buffer_capacity_smem, msg_buffer2[i]);
        }
    }
    ptr_append(msg_buffer, msg_buffer_index, msg_buffer_capacity_smem, 0);
    u64 msg_buffer_len = strlen(msg_buffer);
    msg_buffer_utf8.count = msg_buffer_index;
    if (context.log_print) printf("%.*s", utf8_fmt(msg_buffer_utf8));
    if (context.log_file != NULL) fwrite(msg_buffer_utf8.data, 1, msg_buffer_utf8.count, context.log_file);
    log_clear();
}

void log_msg_context(const char* msg, Log_Level level, utf8 sub_string, Cap_File* file) {
    utf8 msg_utf8 = utf8_str(msg);
    log_utf8_context(msg_utf8, level, sub_string, file);
}

void log_utf8_context(utf8 msg, Log_Level level, utf8 sub_string, Cap_File* file) {
    Log log = {0};
    log.msg = msg;
    log.level = level;
    log.sub_string = sub_string;
    log.file = file;
    ptr_append(context.logs, context.logs_count, context.logs_capacity, log);
}

void log_msg_token(const char* msg, Log_Level level, Token token, Cap_File* file) {
    utf8 msg_utf8 = utf8_str(msg);
    log_utf8_token(msg_utf8, level, token, file);
}

void log_msg_ast(const char* msg, Log_Level level, Ast* ast) {
    utf8 msg_utf8 = utf8_str(msg);
    log_utf8_ast(msg_utf8, level, ast);
}

void log_utf8_ast(utf8 msg, Log_Level level, Ast* ast) {
    if (ast == NULL) return log_utf8(msg, level);
    utf8 combined_sub_string = {0};
    if (ast->tokens.count != 0) {
        combined_sub_string.data = ast->tokens.data[0].data.data;
        char* end_ptr = ast->tokens.data[ast->tokens.count - 1].data.data + ast->tokens.data[ast->tokens.count - 1].data.count;
        combined_sub_string.count = end_ptr - combined_sub_string.data;
    }

    Log log = {0};
    log.msg = msg;
    log.level = level;
    log.sub_string = combined_sub_string;
    log.file = ast->file;
    log.ast = ast;
    ptr_append(context.logs, context.logs_count, context.logs_capacity, log);
}

void log_msg_ssa(const char* msg, Log_Level level, SSA* ssa) {
    utf8 msg_utf8 = utf8_str(msg);
    log_utf8_ssa(msg_utf8, level, ssa);
}

void log_append(Log* log) {
    ptr_append(context.logs, context.logs_count, context.logs_capacity, *log);
}

void log_utf8_ssa(utf8 msg, Log_Level level, SSA* ssa) {
    if (ssa == NULL) return log_utf8(msg, level);
    Ast* ast = ssa->ast;
    utf8 combined_sub_string = {0};
    if (ast != NULL && ast->tokens.count != 0) {
        combined_sub_string.data = ast->tokens.data[0].data.data;
        char* end_ptr = ast->tokens.data[ast->tokens.count - 1].data.data + ast->tokens.data[ast->tokens.count - 1].data.count;
        combined_sub_string.count = end_ptr - combined_sub_string.data;
    }

    Log log = {0};
    log.msg = msg;
    log.level = level;
    log.sub_string = combined_sub_string;
    if (ast == NULL) {
        log.file = NULL;
    } else {
        log.file = ast->file;
    }
    log.ssa = ssa;
    ptr_append(context.logs, context.logs_count, context.logs_capacity, log);
}

void log_utf8_token(utf8 msg, Log_Level level, Token token, Cap_File* file) {
    Log log = {0};
    log.msg = msg;
    log.level = level;
    log.sub_string = token.data;
    log.file = file;
    log.token = token;
    ptr_append(context.logs, context.logs_count, context.logs_capacity, log);
}

void log_msg(const char* msg, Log_Level level) {
    utf8 msg_utf8 = utf8_str(msg);
    log_utf8(msg_utf8, level);
}

void log_utf8(utf8 msg, Log_Level level) {
    Log log = {0};
    log.msg = msg;
    log.level = level;
    ptr_append(context.logs, context.logs_count, context.logs_capacity, log);
}

bool log_has_location(Log* log) {
    return log->sub_string.count > 0;
}
