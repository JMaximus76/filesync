#ifndef JFS_FILE_WALK_H
#define JFS_FILE_WALK_H

#include "file_io.h"
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>

typedef struct jfs_fw_file jfs_fw_file_t;
typedef struct jfs_fw_dir  jfs_fw_dir_t;

typedef struct jfs_fw_state  jfs_fw_state_t; // defined in c file
typedef struct jfs_fw_record jfs_fw_record_t;

typedef enum { JFS_FW_REG, JFS_FW_DIR } jfs_fw_types_t;

struct jfs_fw_state;

struct jfs_fw_file {
    jfs_fw_types_t type;
    jfs_fio_name_t name;
    ino_t          inode;
};

struct jfs_fw_dir {
    jfs_fio_path_t path;
    jfs_fw_file_t *files;
    size_t         file_count;
};

struct jfs_fw_record {
    size_t         dir_count;
    jfs_fw_dir_t  *dir_array;
};

void jfs_fw_file_free(jfs_fw_file_t *file_free);
void jfs_fw_file_transfer(jfs_fw_file_t *file_init, jfs_fw_file_t *file_free);

void jfs_fw_dir_free(jfs_fw_dir_t *dir_free);
void jfs_fw_dir_transfer(jfs_fw_dir_t *dir_init, jfs_fw_dir_t *dir_free);

JFS_ERR jfs_fw_state_create(jfs_fw_state_t **state_take, const jfs_fio_path_t *start_path);
void    jfs_fw_state_destroy(jfs_fw_state_t **state_give);
JFS_ERR jfs_fw_state_step(jfs_fw_state_t *state, int *done_flag);

JFS_ERR jfs_fw_record_init(jfs_fw_record_t *record_init, jfs_fw_state_t **state_give);
void    jfs_fw_record_free(jfs_fw_record_t *record_free);

#endif
