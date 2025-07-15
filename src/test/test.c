#include "file_walk.h"
#include "error.h"

#include <stdio.h>
#include <time.h>

jfs_error_t some_function() {
    jfs_error_t err;

    jfs_fw_state_t *state;
    CHECK_ERR(jfs_fw_state_create("/home/jacob", 1, &state));

    while (jfs_fw_walk_check(state)) {
        err = jfs_fw_walk_step(state);
        if (err == JFS_FW_ERR_SKIP) continue;
        GOTO_ERR(err, clean);
    }

    jfs_fw_walk_t *walk;
    GOTO_ERR_SET(err, jfs_fw_walk_create(&state, &walk), clean);
    jfs_fw_walk_free(&walk);

    return JFS_OK;
clean:
    jfs_fw_state_free(&state);
    RETURN_ERR(err);
}

int main() {
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    some_function();

    clock_gettime(CLOCK_MONOTONIC, &end);

    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    long microseconds_total = seconds * 1000000 + nanoseconds / 1000;

    printf("Elapsed time: %ld microseconds\n", microseconds_total);
    return 0;
}
