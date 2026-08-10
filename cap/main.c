#include "src/cap.c"
#include "src/log.h"

#pragma comment(linker, "/STACK:0x100000")

Context context = {0};
int main(int argc, char* argv[]) {
    // float f = 2147483647.0f;
    // printf("%f\n", f);
    // printf("%d\n", INT_MIN);
    // printf("%d\n", INT_MAX);
    // debug_break();
    // return 0;
    // i32 b = -123;
    // i64 c = 0;
    // i64* b2 = arbitrary_int_cast(&b, 32, 64, true);
    // i32* c2 = arbitrary_int_cast(&c, 64, 32, true);
    // f64 b3 = arbitrary_int_cast_to_float(&b, 32, true);
    // f64 c3 = arbitrary_int_cast_to_float(&c, 64, true);
    //
    // utf8 b_str = arbitrary_int_to_string(&b, 32, true);
    // utf8 c_str = arbitrary_int_to_string(&c, 64, true);
    // printf("b_str: %.*s\n", utf8_fmt(b_str));
    // printf("c_str: %.*s\n", utf8_fmt(c_str));
    //
    // printf("b3: %f\n", b3);
    // printf("c3: %f\n", c3);
    // printf("b: %d\n", b);
    // for (u32 i = 0; i < sizeof(i32) * 8; i++) {
    //     bool bit = bit_get(&b, i);
    //     printf("%d ", bit);
    // }
    // printf("\n");
    // printf("b2: %lld\n", *b2);
    // for (u32 i = 0; i < sizeof(i64) * 8; i++) {
    //     bool bit = bit_get(b2, i);
    //     printf("%d ", bit);
    // }
    // printf("\n");
    // printf("c: %lld\n", c);
    // for (u32 i = 0; i < sizeof(i64) * 8; i++) {
    //     bool bit = bit_get(&c, i);
    //     printf("%d ", bit);
    // }
    // printf("\n");
    // printf("c2: %d\n", *c2);
    // for (u32 i = 0; i < sizeof(i32) * 8; i++) {
    //     bool bit = bit_get(c2, i);
    //     printf("%d ", bit);
    // }
    // debug_break();
    // return 0;

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
    // log_output(log_debug);
    log_output(log_info);
    f64 end_time = get_time_in_seconds();
    f64 elapsed_time = end_time - start_time;
    printf("Elapsed time: %.8f seconds\n", elapsed_time);

    printf("-----------------------------------------------\n");
    printf("--------    Running Test Executable    --------\n");
    printf("-----------------------------------------------\n");
    int run_res = system("test.exe");
    printf("-----------------------------------------------\n");
    printf("-------- Done Running Test Executable ---------\n");
    printf("-----------------------------------------------\n");
    printf("exit code: %d\n", run_res);

    f64 end_run_time = get_time_in_seconds();
    f64 elapsed_run_time = end_run_time - end_time;
    printf("Elapsed time: %.8f seconds\n", elapsed_run_time);

    remove("test.exe");

    debug_break();
    return 0;
}
