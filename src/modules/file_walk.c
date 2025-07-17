#include "file_walk.h"
#include "error.h"
#include "file_io.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct fw_file_node  fw_file_node_t;
typedef struct fw_file_stack fw_file_stack_t;

struct fw_file_node {
    fw_file_node_t *next;
    jfs_fw_file_t  *file;
};

struct fw_file_stack {
    jfs_fw_id_t    *id;
    size_t          file_count;
    fw_file_node_t *head;
};

static JFS_ERR fw_file_stack_create(jfs_fw_id_t **id_give, fw_file_stack_t **stack_take);
static void    fw_file_stack_free(fw_file_stack_t **stack_give);
static JFS_ERR fw_file_stack_push(fw_file_stack_t *stack, jfs_fw_file_t **file_give);

static JFS_ERR fw_file_stack_to_dir(fw_file_stack_t **stack_give, jfs_fw_dir_t **dir_take);
static void    fw_dir_free(jfs_fw_dir_t **dir_give);

static JFS_ERR fw_id_create(const char *path, ino_t inode, jfs_fw_id_t **id_take);
static JFS_ERR fw_file_create(const char *file_name, const struct stat *file_stat, jfs_fw_file_t **file_take);

static JFS_ERR fw_state_id_push(jfs_fw_state_t *state, jfs_fw_id_t **id_give);
static JFS_ERR fw_state_id_pop(jfs_fw_state_t *state, jfs_fw_id_t **id_take);
static JFS_ERR fw_state_dir_push(jfs_fw_state_t *state, jfs_fw_dir_t **dir_give);

jfs_error_t jfs_fw_state_create(const char *path, ino_t inode, jfs_fw_state_t **state_take) {
    jfs_error_t err;
    struct stat st;
    FAIL_IF(lstat(path, &st) != 0, jfs_err_lstat(errno));
    FAIL_IF(!S_ISDIR(st.st_mode), JFS_ERR_NOT_DIR);

    *state_take = malloc(sizeof(**state_take));
    jfs_fw_state_t *state = *state_take;
    FAIL_IF(state == NULL, JFS_ERR_RESOURCE);

    state->dir_count = 0;
    state->id_count = 0;
    state->dir_head = NULL;
    state->id_head = NULL;

    jfs_fw_id_t *id;
    err = fw_id_create(path, inode, &id);
    if (err != JFS_OK) {
        jfs_fw_state_free(state_take);
        RETURN_ERR(JFS_ERR_RESOURCE);
    }

    err = fw_state_id_push(state, &id);
    if (err != JFS_OK) {
        free(id);
        jfs_fw_state_free(state_take);
        RETURN_ERR(JFS_ERR_RESOURCE);
    }

    return JFS_OK;
}

void jfs_fw_state_free(jfs_fw_state_t **state_give) {
    if (state_give == NULL || *state_give == NULL) {
        return;
    }
    jfs_fw_state_t *state = *state_give;

    jfs_fw_id_node_t *id_node = state->id_head;
    while (id_node != NULL) {
        jfs_fw_id_node_t *temp = id_node->next;
        free(id_node->id);
        free(id_node);
        id_node = temp;
    }

    jfs_fw_dir_node_t *dir_node = state->dir_head;
    while (dir_node != NULL) {
        jfs_fw_dir_node_t *temp = dir_node->next;
        fw_dir_free(&dir_node->dir);
        free(dir_node);
        dir_node = temp;
    }

    free(state);
    *state_give = NULL;
}

// closedir() is never checked for error (i don't care)
jfs_error_t jfs_fw_walk_step(jfs_fw_state_t *state) {
    jfs_error_t err;

    jfs_fw_id_t *dir_id;
    CHECK_ERR(fw_state_id_pop(state, &dir_id));

    fw_file_stack_t *file_stack;
    err = fw_file_stack_create(&dir_id, &file_stack);
    if (err != JFS_OK) {
        free(dir_id);
        RETURN_ERR(err);
    }

    DIR *s_dir = opendir(file_stack->id->path);
    if (s_dir == NULL) {
        fw_file_stack_free(&file_stack);
        err = jfs_err_opendir(errno);
        if (err == JFS_ERR_ACCESS) {
            err = JFS_FW_ERR_SKIP;
        }
        RETURN_ERR(err);
    }

    errno = 0;
    struct dirent *ent;
    char           path_buf[PATH_BUF];
    strcpy(path_buf, file_stack->id->path);
    size_t path_end = strlen(path_buf);
    while ((ent = readdir(s_dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        GOTO_ERR_SET(err, jfs_fio_change_path(path_buf, path_end, ent->d_name), cleanup);

        struct stat st;
        if (lstat(path_buf, &st) != 0) {
            err = jfs_err_lstat(errno);
            if (err == JFS_ERR_ACCESS) {
                continue;
            }
            GOTO_ERR(err, cleanup);
        }

        if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)) {
            jfs_fw_file_t *file;
            GOTO_ERR_SET(err, fw_file_create(ent->d_name, &st, &file), cleanup);
            GOTO_ERR_SET(err, fw_file_stack_push(file_stack, &file), cleanup);
            if (S_ISDIR(st.st_mode)) {
                jfs_fw_id_t *walk_id;
                GOTO_ERR_SET(err, fw_id_create(path_buf, st.st_ino, &walk_id), cleanup);
                GOTO_ERR_SET(err, fw_state_id_push(state, &walk_id), cleanup);
            }
        }
    }
    if (errno != 0) {
        GOTO_ERR_SET(err, JFS_ERR_BAD_FD, cleanup);
    }
    closedir(s_dir);

    jfs_fw_dir_t *dir;
    err = fw_file_stack_to_dir(&file_stack, &dir);
    if (err != JFS_OK) {
        fw_file_stack_free(&file_stack);
        RETURN_ERR(err);
    }

    err = fw_state_dir_push(state, &dir);
    if (err != JFS_OK) {
        fw_dir_free(&dir);
        RETURN_ERR(err);
    }

    return JFS_OK;

cleanup:
    closedir(s_dir);
    fw_file_stack_free(&file_stack);
    RETURN_ERR(err);
}

int jfs_fw_walk_check(jfs_fw_state_t *state) {
    return state->id_count > 0;
}

jfs_error_t jfs_fw_walk_create(jfs_fw_state_t **state_give, jfs_fw_walk_t **walk_take) {
    jfs_fw_state_t *state = *state_give;
    FAIL_IF(state->dir_count == 0 || state->id_count > 0, JFS_FW_ERR_STATE);

    *walk_take = malloc(sizeof(**walk_take));
    jfs_fw_walk_t *walk = *walk_take;
    FAIL_IF(walk == NULL, JFS_ERR_RESOURCE);

    walk->count = state->dir_count;

    walk->dir_arr = malloc(walk->count * sizeof(*(walk->dir_arr)));
    if (walk->dir_arr == NULL) {
        free(walk);
        *walk_take = NULL;
        RETURN_ERR(JFS_ERR_RESOURCE);
    }

    jfs_fw_dir_node_t *node = state->dir_head;
    for (size_t i = 0; i < walk->count; i++) {
        walk->dir_arr[i] = *node->dir;
        node->dir->file_arr = NULL;
        node->dir->id = NULL;
        node = node->next;
    }

    jfs_fw_state_free(state_give);
    return JFS_OK;
}

void jfs_fw_walk_free(jfs_fw_walk_t **walk_give) {
    if (walk_give == NULL || *walk_give == NULL) {
        return;
    }

    jfs_fw_walk_t *walk = *walk_give;
    for (size_t i = 0; i < walk->count; i++) {
        jfs_fw_dir_t *dir = &walk->dir_arr[i];
        free(dir->file_arr);
        free(dir->id);
    }

    free(walk->dir_arr);
    free(walk);
    *walk_give = NULL;
}

static jfs_error_t fw_file_stack_create(jfs_fw_id_t **id_give, fw_file_stack_t **stack_take) {
    *stack_take = malloc(sizeof(**stack_take));
    fw_file_stack_t *list = *stack_take;
    FAIL_IF(list == NULL, JFS_ERR_RESOURCE);

    list->file_count = 0;
    list->head = NULL;
    list->id = *id_give;
    *id_give = NULL;

    return JFS_OK;
}

static void fw_file_stack_free(fw_file_stack_t **stack_give) {
    if (stack_give == NULL || *stack_give == NULL) {
        return;
    }

    fw_file_stack_t *stack = *stack_give;
    free(stack->id);

    fw_file_node_t *node = stack->head;
    while (node != NULL) {
        fw_file_node_t *temp = node->next;
        free(node->file);
        free(node);
        node = temp;
    }

    free(stack);
    *stack_give = NULL;
}

static jfs_error_t fw_file_stack_push(fw_file_stack_t *stack, jfs_fw_file_t **file_give) {
    fw_file_node_t *node = malloc(sizeof(*node));
    FAIL_IF(node == NULL, JFS_ERR_RESOURCE);

    node->file = *file_give;
    *file_give = NULL;

    node->next = stack->head;
    stack->head = node;
    stack->file_count += 1;

    return JFS_OK;
}

static jfs_error_t fw_file_stack_to_dir(fw_file_stack_t **stack_give, jfs_fw_dir_t **dir_take) {
    *dir_take = malloc(sizeof(**dir_take));
    jfs_fw_dir_t *dir = *dir_take;
    FAIL_IF(dir == NULL, JFS_ERR_RESOURCE);

    fw_file_stack_t *stack = *stack_give;
    dir->file_count = stack->file_count;
    if (dir->file_count > 0) {
        dir->file_arr = malloc(dir->file_count * sizeof(*(dir->file_arr)));
        if (dir->file_arr == NULL) {
            free(dir);
            *dir_take = NULL;
            RETURN_ERR(JFS_ERR_RESOURCE);
        }

        fw_file_node_t *node = stack->head;
        for (size_t i = 0; i < dir->file_count; i++) {
            dir->file_arr[i] = *node->file;
            node = node->next;
        }
    } else {
        dir->file_arr = NULL;
    }

    dir->id = stack->id;
    stack->id = NULL; // so the id doesn't get freed on fw_file_stack_free()
    fw_file_stack_free(stack_give);
    return JFS_OK;
}

static void fw_dir_free(jfs_fw_dir_t **dir_give) {
    if (dir_give == NULL || *dir_give == NULL) {
        return;
    }

    jfs_fw_dir_t *dir = *dir_give;
    free(dir->file_arr);
    free(dir->id);
    free(dir);
    *dir_give = NULL;
}

static jfs_error_t fw_id_create(const char *path, ino_t inode, jfs_fw_id_t **id_take) {
    FAIL_IF(strlen(path) > PATH_MAX, JFS_ERR_PATH_LEN);

    *id_take = malloc(sizeof(**id_take));
    jfs_fw_id_t *id = *id_take;
    FAIL_IF(id == NULL, JFS_ERR_RESOURCE);

    id->inode = inode;
    snprintf(id->path, sizeof(id->path), "%s", path);

    return JFS_OK;
}

static jfs_error_t fw_file_create(const char *file_name, const struct stat *file_stat, jfs_fw_file_t **file_take) {
    FAIL_IF(strlen(file_name) > NAME_MAX, JFS_ERR_NAME_LEN);

    *file_take = malloc(sizeof(**file_take));
    jfs_fw_file_t *file = *file_take;
    FAIL_IF(file == NULL, JFS_ERR_RESOURCE);

    strcpy(file->name, file_name);
    file->inode = file_stat->st_ino;
    file->mode = file_stat->st_mode;
    file->size = file_stat->st_size;
    file->mod_time = file_stat->st_mtim;

    return JFS_OK;
}

static jfs_error_t fw_state_id_push(jfs_fw_state_t *state, jfs_fw_id_t **id_give) {
    jfs_fw_id_node_t *node = malloc(sizeof(*node));
    FAIL_IF(node == NULL, JFS_ERR_RESOURCE);

    node->id = *id_give;
    *id_give = NULL;

    node->next = state->id_head;
    state->id_head = node;
    state->id_count += 1;

    return JFS_OK;
}

static jfs_error_t fw_state_id_pop(jfs_fw_state_t *state, jfs_fw_id_t **id_take) {
    FAIL_IF(state->id_count <= 0, JFS_ERR_EMPTY_STACK);

    jfs_fw_id_node_t *node = state->id_head;
    state->id_head = node->next;
    state->id_count -= 1;

    *id_take = node->id;
    node->id = NULL;

    free(node);

    return JFS_OK;
}

static jfs_error_t fw_state_dir_push(jfs_fw_state_t *state, jfs_fw_dir_t **dir_give) {
    jfs_fw_dir_node_t *node = malloc(sizeof(*node));
    FAIL_IF(node == NULL, JFS_ERR_RESOURCE);

    node->dir = *dir_give;
    *dir_give = NULL;

    node->next = state->dir_head;
    state->dir_head = node;
    state->dir_count += 1;

    return JFS_OK;
}
