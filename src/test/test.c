#include "error.h"
#include "file_walk.h"
#include <dirent.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>

void timed_test_function(jfs_fw_record_t *record, jfs_err_t *err) {
    jfs_fio_path_t  path = {0};
    jfs_fw_state_t *state = NULL;

    jfs_fio_path_init(&path, "/home/jacob/code/filesync/", err);
    GOTO_IF_ERR(cleanup);

    state = jfs_fw_state_create(&path, err);
    GOTO_IF_ERR(cleanup);

    while (jfs_fw_state_step(state, err)) {
        if (*err == JFS_ERR_FW_SKIP) {
            RES_ERR;
            continue;
        }
        GOTO_IF_ERR(cleanup);
    }

    jfs_fw_record_init(record, &state, err);
    GOTO_IF_ERR(cleanup);

    return;
cleanup:
    jfs_fio_path_free(&path);
    jfs_fw_state_destroy(&state);
    RETURN_VOID_ERR;
}

int main() {
    struct timespec start;
    struct timespec end;

    jfs_fw_record_t record = {0};
    jfs_err_t       err = JFS_OK;

    clock_gettime(CLOCK_MONOTONIC, &start);
    timed_test_function(&record, &err);
    clock_gettime(CLOCK_MONOTONIC, &end);

    size_t total_files = 0;
    for (size_t i = 0; i < record.dir_count; i++) {
        printf("%s\n", record.dir_array[i].path.str);
        total_files += record.dir_array[i].file_count;
    }
    printf("Number of dirs walked:  %zu\n", record.dir_count);
    printf("Number of files walked: %zu\n", total_files);

    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    long microseconds_total = (seconds * 1000000) + (nanoseconds / 1000); // NOLINT
    printf("Elapsed time: %ld microseconds\n", microseconds_total);

    return 0;
}
