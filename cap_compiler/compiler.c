#include <time.h>

#include "cap.h"

i32 main(int argc, char** argv) {
    clock_t start = clock();

    cap_init();
    Cap_Project project = cap_create_project(str("C:/Users/brevi/dev/cap/examples/basic"));
    Cap_Folder folder = project.base_folder;
    Cap_File* files = folder.files;
    u64 files_count = folder.files_count;
    for (u64 i = 0; i < files_count; i++) {
        Cap_File file = files[i];
        String path = file.path;
        String tokens_str = token_tokens_to_string(file.tokens);
        printf("%.*s\n\n", str_info(path));
        printf("Tokens:\n");
        printf("%.*s\n\n", str_info(tokens_str));
    }

    clock_t end = clock();

    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("CPU time: %f seconds\n", cpu_time);

    return 0;
}
