#include "cap.h"

String filesystem_read_file(String path) {
    FILE* file = fopen(path.data, "rb");
    if (file == NULL) {
        char buffer[4096];
        sprintf(buffer, "Failed to open file: %.*s", str_info(path));
        String msg = string_create(buffer, strlen(buffer));
        mabort(msg);
    }
    fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* mem = cap_alloc(size);
    fread(mem, 1, size, file);

    fclose(file);

    String str = string_create(mem, size);
    return str;
}
