#include <stdio.h>

#include "base/basic.h"
#include "cap.h"
#include "cap/tokens.h"

int main(int argc, char** argv) {
    double time_start = get_time();

    init_base_context();
    Project* project = project_create("C:/Users/brevi/dev/cap/scratch");
    double time_end = get_time();
    green_printf("Time taken: %f\n", time_end - time_start);

    green_printf("Done creating project\n");
    for (u64 i = 0; i < project->files.count; i++) {
        File* file = *File_Ptr_List_get(&project->files, i);
        printf("-----------------\n");
        printf("%s\n", file->path);
        printf("-----------------\n");
        while (file->tokens[i].type != tt_eof) {
            token_print(&file->tokens[i]);
            i++;
        }
        printf("\n-----------------\n");
    }

    double time_start_semantic = get_time();
    project_semantic_analysis(project);
    time_end = get_time();
    green_printf("\nDone semantic analysis\n");
    green_printf("Time taken: %f\n", time_end - time_start_semantic);

    double time_start_compile = get_time();
    project_compile_llvm(project);
    time_end = get_time();
    green_printf("\nDone compiling llvm\n");
    green_printf("Time taken: %f\n", time_end - time_start_compile);
    green_printf("Total Time taken: %f\n", time_end - time_start);

    DEBUGBREAK();

    return 0;
}
