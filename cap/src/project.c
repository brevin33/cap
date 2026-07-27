#include "cap.h"
#include "ssa.h"

utf8 cap_file_get_line(Cap_File* file, u32 line) {
    u32 current_line = 1;
    utf8 walk_to_line_start = file->contents;
    while (current_line < line) {
        u32 unicode = utf8_get(walk_to_line_start);
        utf8_next(&walk_to_line_start);
        if (unicode == '\n') {
            current_line++;
        }
    }

    utf8 clip_till_line_end = walk_to_line_start;
    u32 unicode = utf8_get(clip_till_line_end);
    utf8_next(&clip_till_line_end);
    char* line_end = NULL;
    while (true) {
        if (unicode == '\n' || unicode == 0) {
            line_end = clip_till_line_end.data;
            break;
        }
        unicode = utf8_get(clip_till_line_end);
        utf8_next(&clip_till_line_end);
    }

    u32 line_length = line_end - walk_to_line_start.data - 1;
    utf8 line_contents = {walk_to_line_start.data, line_length};
    return line_contents;
}

Cap_Folder cap_folder_create_from_path(utf8 path) {
    DIR* dir = opendir(path.data);
    Cap_Folder folder = {0};
    if (dir == NULL) {
        char buffer[4096];
        sprintf(buffer, "Failed to open directory: %.*s", utf8_fmt(path));
        log_msg(buffer, log_error);
        return folder;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        char buffer[4096];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char* filepath = entry->d_name;
        struct stat statbuf;
        if (stat(filepath, &statbuf) == 0) {
            if (S_ISREG(statbuf.st_mode)) {
                char* extension = strrchr(entry->d_name, '.');
                if (extension == NULL) continue;
                if (strcmp(extension, ".cap") != 0) continue;
                char* file_contents_char = read_file(filepath);
                utf8 file_contents = utf8_str(file_contents_char);
                if (file_contents.data == NULL) continue;
                u32 filepath_len = strlen(filepath);
                char* file_path_copy = alloc(filepath_len + 1);
                memcpy(file_path_copy, filepath, filepath_len + 1);

                Cap_File file = {0};
                file.path = utf8_str(file_path_copy);
                file.contents = file_contents;
                file.tokens = tokens_from_file(&file);
                if (log_has_error()) continue;
                file.ast = ast_create_from_file(&file);
                ptr_append(folder.files, folder.file_count, folder.file_capacity, file);
            } else if (S_ISDIR(statbuf.st_mode)) {
                // Ignore for now
            }
        }
    }
    closedir(dir);

    // never go to semantic analysis if we failed in tokenization or ast creation
    if (log_has_error()) return (Cap_Folder){0};

    Ast** top_level_statements = NULL;
    u32 top_level_statements_count = 0;
    u32 top_level_statements_capacity = 0;

    for (u32 i = 0; i < folder.file_count; i++) {
        Cap_File* file = folder.files + i;
        Ast* ast = &file->ast;
        assert(ast->kind == Ast_Kind_File);
        for (u32 j = 0; j < ast->data.file.ast_count; j++) {
            Ast* top_level_statement = &ast->data.file.top_level_statements[j];
            ptr_append(top_level_statements, top_level_statements_count, top_level_statements_capacity, top_level_statement);
        }
    }

    Scope namespace_scope = {0};
    namespace_scope.ast = NULL;
    namespace_scope.parent = &context.intrinsic_scope;

    Ast** semantic_parse_order = alloc(sizeof(Ast*) * top_level_statements_count);
    u32 semantic_parse_order_count = 0;

    bool changed = true;
    while (changed && semantic_parse_order_count < top_level_statements_count) {
        changed = false;
        for (u32 i = 0; i < top_level_statements_count; i++) {
            Ast* top_level_statement = top_level_statements[i];
            bool already_added = false;
            for (u32 j = 0; j < semantic_parse_order_count; j++) {
                Ast* semantic_parse_order_top_level_statement = semantic_parse_order[j];
                if (top_level_statement == semantic_parse_order_top_level_statement) {
                    already_added = true;
                    break;
                }
            }
            if (already_added) continue;
            u32 log_location = log_end_location();
            if (!ast_top_level_prototype_resolve_variables(top_level_statement, &namespace_scope)) {
                log_clear_after(log_location);
                continue;
            }
            changed = true;
            semantic_parse_order[semantic_parse_order_count] = top_level_statement;
            semantic_parse_order_count++;
        }
    }
    if (semantic_parse_order_count != top_level_statements_count) {
        // run again to log errors
        for (u32 i = 0; i < top_level_statements_count; i++) {
            Ast* top_level_statement = top_level_statements[i];
            bool already_added = false;
            for (u32 j = 0; j < semantic_parse_order_count; j++) {
                Ast* semantic_parse_order_top_level_statement = semantic_parse_order[j];
                if (top_level_statement == semantic_parse_order_top_level_statement) {
                    already_added = true;
                    break;
                }
            }
            if (already_added) continue;
            bool success = ast_top_level_prototype_resolve_variables(top_level_statement, &namespace_scope);
            assert(!success);
        }
        log_msg("Failed to resolve variables exiting early", log_error);
        return folder;
    }

    for (u32 i = 0; i < semantic_parse_order_count; i++) {
        Ast* top_level_statement = semantic_parse_order[i];
        if (!ast_top_level_implement_resolve_variables(top_level_statement, &namespace_scope)) {
            log_msg("Failed to resolve variables exiting early", log_error);
            return folder;
        }
    }

    SSA_Block* block = &context.global_block;
    for (u32 i = 0; i < semantic_parse_order_count; i++) {
        Ast* top_level_statement = semantic_parse_order[i];
        SSA* ssa = ssa_top_level_ast_to_ssa(top_level_statement, block);
    }

    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = block->statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* ssa = list->statements + j;
            ssa_top_level_post_parse(ssa, block);
        }
    }

    utf8 str = ssa_recursive_get_block_strings(block);
    printf("%.*s", utf8_fmt(str));

    if (!ssa_type_check_block(block)) return folder;
    if (!ssa_type_check_builds()) return folder;

    if (!ssa_run_block(block)) return folder;
    if (!ssa_run_builds()) return folder;

    return folder;
}
