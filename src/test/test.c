#include "file_walk.h"
#include "error.h"

#include <stdio.h>
#include <inttypes.h>
#include <time.h>

void jfs_fw_walk_print(const jfs_fw_walk_t *walk) {
    if (!walk) {
        printf("walk is NULL\n");
        return;
    }
    printf("jfs_fw_walk: %zu directories\n", walk->count);

    for (size_t i = 0; i < walk->count; i++) {
        const jfs_fw_dir_t *dir = &walk->dir_arr[i];
        // Print directory ID info
        printf("  [Dir %zu]\n", i);
        printf("    path: \"%s\"\n", dir->id->path);
        printf("    inode: %" PRIuMAX "\n", (uintmax_t)dir->id->inode);
        printf("    contains %zu entries:\n", dir->file_count);

        // Print each file in the directory
        for (size_t j = 0; j < dir->file_count; j++) {
            const jfs_fw_file_t *f = &dir->file_arr[j];
            // format modification time
            char timestr[64] = {0};
            struct tm tm;
            localtime_r(&f->mod_time.tv_sec, &tm);
            strftime(timestr, sizeof timestr, "%Y-%m-%d %H:%M:%S", &tm);

            printf("      [%zu] name=\"%s\", inode=%" PRIuMAX ", mode=0%o, size=%jd, mtime=%s\n",
                   j,
                   f->name,
                   (uintmax_t)f->inode,
                   f->mode & 07777,
                   (intmax_t)f->size,
                   timestr);
        }
    }
}

JFS_ERR some_function(jfs_fw_walk_t **walk_take) {
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

    *walk_take = walk;

    return JFS_OK;
clean:
    jfs_fw_state_free(&state);
    RETURN_ERR(err);
}


int main() {
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    jfs_fw_walk_t *walk;
    jfs_error_t err = some_function(&walk);

    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("number of dirs: %zu\n", walk->count);

    jfs_fw_walk_free(&walk);

    if (err != JFS_OK) {
        printf("error: %s\n", jfs_err_to_string(err));
    }

    long seconds = end.tv_sec - start.tv_sec;
    long nanoseconds = end.tv_nsec - start.tv_nsec;
    long microseconds_total = seconds * 1000000 + nanoseconds / 1000;

    printf("Elapsed time: %ld microseconds\n", microseconds_total);

    return 0;
}
