#include "src/cap.c"

Context context = {0};
int main(int argc, char* argv[]) {
    f64 start_time = get_time_in_seconds();
    char* dir = "./";
    if (argc > 1) {
        dir = argv[1];
    }
    utf8 dir_utf8 = utf8_str(dir);
    int res = cap_init(dir_utf8);
    if (res != 0) {
        return res;
    }
    cap_analyze(dir_utf8);
    log_output(log_info);
    // log_output(log_debug);
    f64 end_time = get_time_in_seconds();
    f64 elapsed_time = end_time - start_time;
    printf("Elapsed time: %.8f seconds\n", elapsed_time);

    printf("---------------------------------------------\n");
    printf("------    Running Test Executable    --------\n");
    printf("---------------------------------------------\n");
    int run_res = system("test.exe");
    printf("---------------------------------------------\n");
    printf("------ Done Running Test Executable ---------\n");
    printf("---------------------------------------------\n");
    printf("exit code: %d\n", run_res);

    f64 end_run_time = get_time_in_seconds();
    f64 elapsed_run_time = end_run_time - end_time;
    printf("Elapsed time: %.8f seconds\n", elapsed_run_time);

    debug_break();
    return 0;
}
