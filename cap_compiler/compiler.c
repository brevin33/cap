#include <time.h>

#include "cap.h"

i32 main(int argc, char** argv) {
    clock_t start = clock();

    init_cap();

    Cap_File file = file_create_cap_file(str("C:/Users/brevi/dev/cap/examples/basic.cap"));

    String tokens_str = token_tokens_to_string(file.tokens);
    printf("Tokens:\n");
    printf("%.*s\n\n", str_info(tokens_str));

    clock_t end = clock();

    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("CPU time: %f seconds\n", cpu_time);

    return 0;
}
