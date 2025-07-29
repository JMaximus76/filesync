#include "error.h"
#include "file_walk.h"
#include "thread_message.h"
#include <dirent.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

struct test_times {
    struct timespec start;
    struct timespec stop;
};

void start_time(struct test_times *times);
void stop_time(struct test_times *times);
void print_time(const struct test_times *times);
void print_status(const char *test_name, jfs_err_t *err);

void file_walk_test(int verbose_flag, jfs_err_t *err);

void start_time(struct test_times *times) {
    clock_gettime(CLOCK_MONOTONIC, &times->start);
}

void stop_time(struct test_times *times) {
    clock_gettime(CLOCK_MONOTONIC, &times->stop);
}

void print_time(const struct test_times *times) {
    long seconds = times->stop.tv_sec - times->start.tv_sec;
    long nanoseconds = times->stop.tv_nsec - times->start.tv_nsec;
    long microseconds_total = (seconds * 1000000) + (nanoseconds / 1000); // NOLINT
    printf("Elapsed time: %ld microseconds\n", microseconds_total);
}

void print_status(const char *test_name, jfs_err_t *err) {
    if (*err == JFS_OK) {
        printf("Success: %s\n", test_name);
    } else {
        printf("Fail: %s\n", test_name);
        RES_ERR;
    }
}

void msg_queue_test(bool verbose_flag, jfs_err_t *err) {
    jfs_tm_queue_t *queue = NULL;

    queue = jfs_tm_queue_create(err);

     
}

void file_walk_test(int verbose_flag, jfs_err_t *err) { // NOLINT
    struct test_times times = {0};
    jfs_fw_record_t   record = {0};
    jfs_fio_path_t    path = {0};
    jfs_fw_state_t   *state = NULL;

    start_time(&times);

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

    jfs_fw_record_init(&record, state, err);
    GOTO_IF_ERR(cleanup);

    stop_time(&times);

    if (verbose_flag) {
        size_t total_files = 0;
        for (size_t i = 0; i < record.dir_count; i++) {
            printf("%s\n", record.dir_array[i].path.str);
            for (size_t j = 0; j < record.dir_array[i].file_count; j++) {
                printf("    %s\n", record.dir_array[i].files[j].name.str);
            }
            total_files += record.dir_array[i].file_count;
        }
        printf("Number of dirs walked:  %zu\n", record.dir_count);
        printf("Number of files walked: %zu\n", total_files);
    }

    print_time(&times);

    jfs_fw_record_free(&record);
    return;
cleanup:
    stop_time(&times);
    jfs_fio_path_free(&path);
    jfs_fw_state_destroy(state);
    VOID_RETURN_ERR;
}

int main() {
    jfs_err_t err = JFS_OK;

    file_walk_test(1, &err);
    print_status("file walk", &err);

    return 0;
}
