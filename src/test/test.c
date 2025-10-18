#include "error.h"
#include "file_walk.h"
#include "thread_safe_slab.h"
#include <dirent.h>
#include <inttypes.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

#define NUMBER_OF_THING   1000000
#define NUMBER_OF_THREADS 10

struct thing {
    pthread_t pt;
};

void *tss_test_thread_malloc(void *arg) {
    (void) arg;
    struct thing *array[NUMBER_OF_THING];
    pthread_t     pt = pthread_self();
    jfs_err_t     err = JFS_OK;

    for (uint32_t i = 0; i < NUMBER_OF_THING; i++) {
        array[i] = jfs_malloc(sizeof(struct thing), &err);
        if (err != JFS_OK) {
            printf("ERROR IN THREAD");
            return NULL;
        }
        array[i]->pt = pt + i;
    }

    bool not_correct = false;
    for (uint32_t i = 0; i < NUMBER_OF_THING; i++) {
        not_correct = not_correct || !(array[i]->pt == pt + i);
        free(array[i]);
    }
    if (not_correct) printf("the array was messed up ig\n");

    return NULL;
}

void *tss_test_thread(void *arg) {
    jfs_tss_cache_t cache = {0};
    jfs_tss_cache_init(&cache, arg);
    struct thing *array[NUMBER_OF_THING];

    pthread_t pt = pthread_self();
    jfs_err_t err = JFS_OK;

    for (uint32_t i = 0; i < NUMBER_OF_THING; i++) {
        array[i] = jfs_tss_alloc(&cache, &err);
        if (err != JFS_OK) {
            printf("ERROR IN THREAD");
            return NULL;
        }

        array[i]->pt = pt + i;
    }

    bool not_correct = false;
    for (uint32_t i = 0; i < NUMBER_OF_THING; i++) {
        not_correct = not_correct || !(array[i]->pt == pt + i);
        jfs_tss_free(&cache, array[i]);
    }
    if (not_correct) printf("the array was messed up ig\n");
    jfs_tss_cache_full_release(&cache);

    return NULL;
}

void tss_test(jfs_err_t *err) {
    struct test_times time = {0};

    start_time(&time);

    jfs_tss_allocator_config_t config = {
        .cache_cap = 1028,                   // NOLINT
        .cache_acquire_amount = 1028,               // NOLINT
        .cache_release_amount = 1028,               // NOLINT
        .obj_align = alignof(struct thing), // NOLINT
        .obj_size = sizeof(struct thing),   // NOLINT
        .pages_per_slab = 64,               // NOLINT
        .retire_threshold = 8,              // NOLINT
    };

    jfs_tss_allocator_t *alloc = jfs_tss_allocator_create(&config, err);
    VOID_CHECK_ERR;
    pthread_t threads[NUMBER_OF_THREADS];
    for (uint32_t i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_create(&threads[i], NULL, tss_test_thread, alloc);
    }

    for (uint32_t i = 0; i < NUMBER_OF_THREADS; i++) {
        void *nothing = NULL;
        pthread_join(threads[i], &nothing);
    }

    jfs_tss_allocator_explicit_retire(alloc);
    jfs_tss_allocator_destroy(alloc);

    stop_time(&time);
    printf("Total time:\n");
    print_time(&time);
    puts("\n");
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

    // file_walk_test(1, &err);
    // print_status("file walk", &err);
    // err = JFS_OK;

    tss_test(&err);
    print_status("tss", &err);
    err = JFS_OK;

    return 0;
}
