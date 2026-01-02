#include "cap.h"

Cap_File file_create_cap_file(String path) {
    Cap_File file = {0};

    file.path = path;
    file.content = filesystem_read_file(path);
    file.tokens = token_tokenize(&file);
    file.ast = ast_parse_tokens(file.tokens, &file);

    return file;
}
