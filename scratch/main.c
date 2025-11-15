#include <stdio.h>

#include "cap.h"
#include "cap/tokens.h"

int main(int argc, char** argv) {
    init_base_context();
    Project* project = project_create("C:/Users/brevi/dev/cap/scratch");

    printf("\nDone building ast\n");
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

    project_semantic_analysis(project);
    printf("\nDone semantic analysis\n");
    project_compile_llvm(project);
    printf("\nDone compiling llvm\n");

    DEBUGBREAK();
    return 0;
}
