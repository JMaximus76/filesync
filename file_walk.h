#ifndef JFS_FILE_WALK_H
#define JFS_FILE_WALK_H

#include "error.h"
#include "file_io.h"
#include <sys/types.h>
#include <limits.h>

typedef struct jfs_fw_id       jfs_fw_id_t;
typedef struct jfs_fw_file     jfs_fw_file_t;
typedef struct jfs_fw_dir      jfs_fw_dir_t;

typedef struct jfs_fw_state    jfs_fw_state_t;
typedef struct jfs_fw_id_node  jfs_fw_id_node_t;
typedef struct jfs_fw_dir_node jfs_fw_dir_node_t;

typedef struct jfs_fw_walk     jfs_fw_walk_t;


struct jfs_fw_id {
    char path[PATH_BUF];
    ino_t inode;
};

struct jfs_fw_file {
    char name[NAME_BUF];
    ino_t inode;
    mode_t mode;
    off_t size;
    struct timespec mod_time;
};

struct jfs_fw_dir {
    jfs_fw_id_t *id;
    size_t file_count;
    jfs_fw_file_t *file_arr;
};

struct jfs_fw_state {
    jfs_fw_id_node_t *id_head;
    size_t id_count;
    jfs_fw_dir_node_t *dir_head;
    size_t dir_count;
};

struct jfs_fw_id_node {
    jfs_fw_id_node_t *next;
    jfs_fw_id_t *id;
};

struct jfs_fw_dir_node {
    jfs_fw_dir_node_t *next;
    jfs_fw_dir_t *dir;
};

struct jfs_fw_walk {
    size_t count;
    jfs_fw_dir_t *dir_arr;
};


JFS_ERR jfs_fw_state_create(const char *path, ino_t inode, jfs_fw_state_t **state_take);
void    jfs_fw_state_free(jfs_fw_state_t **state_give);

JFS_ERR jfs_fw_walk_step(jfs_fw_state_t *state);
int     jfs_fw_walk_check(jfs_fw_state_t *state);
JFS_ERR jfs_fw_walk_create(jfs_fw_state_t **state_give, jfs_fw_walk_t **walk_take);
void    jfs_fw_walk_free(jfs_fw_walk_t **walk_give);

#endif

