#include "file_walk.h"
#include "error.h"
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define FW_PATH_VECTOR_DEFAULT_CAPACITY 16
#define FW_FILE_VECTOR_DEFAULT_CAPACITY 16
#define FW_DIR_VECTOR_DEFAULT_CAPACITY  16

typedef struct fw_path_vector fw_path_vector_t;
typedef struct fw_file_vector fw_file_vector_t;
typedef struct fw_dir_vector  fw_dir_vector_t;

struct fw_path_vector {
    size_t          count;
    size_t          capacity;
    jfs_fio_path_t *path_array;
};

struct fw_file_vector {
    size_t         count;
    size_t         capacity;
    jfs_fw_file_t *file_array;
};

struct fw_dir_vector {
    size_t        count;
    size_t        capacity;
    jfs_fw_dir_t *dir_array;
};

struct jfs_fw_state {
    fw_path_vector_t path_vec;
    fw_dir_vector_t  dir_vec;
};

static JFS_ERR fw_map_dirent_type(unsigned char ent_type, jfs_fw_types_t *fw_type);
static JFS_ERR fw_file_init(jfs_fw_file_t *file_init, const struct dirent *ent);
static JFS_ERR fw_dir_init(jfs_fw_dir_t *dir_init, fw_file_vector_t *vec_free, jfs_fio_path_t *path_free);

static JFS_ERR fw_path_vector_init(fw_path_vector_t *vec_init);
static void    fw_path_vector_free(fw_path_vector_t *vec_free);
static JFS_ERR fw_path_vector_push(fw_path_vector_t *vec, jfs_fio_path_t *path_free);
static JFS_ERR fw_path_vector_pop(fw_path_vector_t *vec, jfs_fio_path_t *path_init);

static JFS_ERR fw_file_vector_init(fw_file_vector_t *vec_init);
static void    fw_file_vector_free(fw_file_vector_t *vec_free);
static JFS_ERR fw_file_vector_push(fw_file_vector_t *vec, jfs_fw_file_t *file_free);
static JFS_ERR fw_file_vector_to_array(jfs_fw_file_t **file_array_take, fw_file_vector_t *vec_free);

static JFS_ERR fw_dir_vector_init(fw_dir_vector_t *vec_init);
static void    fw_dir_vector_free(fw_dir_vector_t *vec_free);
static JFS_ERR fw_dir_vector_push(fw_dir_vector_t *vec, jfs_fw_dir_t *dir_free);
static JFS_ERR fw_dir_vector_to_array(jfs_fw_dir_t **dir_array_take, fw_dir_vector_t *vec_free);

static JFS_ERR fw_scan_dir(DIR *dir, fw_file_vector_t *vec);
static JFS_ERR fw_handle_dirent(const struct dirent *ent, fw_file_vector_t *vec);
static JFS_ERR fw_push_dir_paths(fw_path_vector_t *path_vec, fw_file_vector_t *file_vec, const jfs_fio_path_t *dir_path);

void jfs_fw_file_free(jfs_fw_file_t *file_free) {
    jfs_fio_name_free(&file_free->name);
    memset(file_free, 0, sizeof(*file_free));
}

void jfs_fw_file_transfer(jfs_fw_file_t *file_init, jfs_fw_file_t *file_free) {
    *file_init = *file_free;
    memset(file_free, 0, sizeof(*file_free));
}

void jfs_fw_dir_free(jfs_fw_dir_t *dir_free) {
    if (dir_free->files != NULL) {
        for (size_t i = 0; i < dir_free->file_count; i++) {
            jfs_fw_file_free(&dir_free->files[i]);
        }

        free(dir_free->files);
    }

    jfs_fio_path_free(&dir_free->path);

    memset(dir_free, 0, sizeof(*dir_free));
}

void jfs_fw_dir_transfer(jfs_fw_dir_t *dir_init, jfs_fw_dir_t *dir_free) {
    *dir_init = *dir_free;
    memset(dir_free, 0, sizeof(*dir_free));
}

jfs_error_t jfs_fw_state_create(jfs_fw_state_t **state_take, const jfs_fio_path_t *start_path) {
    jfs_error_t err;

    jfs_fw_state_t *state = NULL;
    jfs_fio_path_t  new_path = {0};

    state = malloc(sizeof(*state));
    FAIL_IF(state == NULL, JFS_ERR_RESOURCE);

    err = fw_path_vector_init(&state->path_vec);
    GOTO_IF_ERR(err, cleanup);

    err = fw_dir_vector_init(&state->dir_vec);
    GOTO_IF_ERR(err, cleanup);

    err = jfs_fio_path_init(&new_path, start_path->str);
    GOTO_IF_ERR(err, cleanup);

    err = fw_path_vector_push(&state->path_vec, &new_path);
    GOTO_IF_ERR(err, cleanup);

    *state_take = state;

    return JFS_OK;
cleanup:
    if (state != NULL) {
        fw_dir_vector_free(&state->dir_vec);
        fw_path_vector_free(&state->path_vec);
        free(state);
    }

    jfs_fio_path_free(&new_path);
    RETURN_ERR(err);
}

void jfs_fw_state_destroy(jfs_fw_state_t **state_give) {
    if (state_give == NULL || *state_give == NULL) return;

    jfs_fw_state_t *state = *state_give;
    fw_path_vector_free(&state->path_vec);
    fw_dir_vector_free(&state->dir_vec);

    free(state);
    *state_give = NULL;
}

jfs_error_t jfs_fw_state_step(jfs_fw_state_t *state, int *done_flag) { // NOLINT(readability-function-cognitive-complexity)
    FAIL_IF(state->path_vec.count == 0, JFS_FW_ERR_STATE);

    jfs_error_t      err;
    fw_file_vector_t file_vec = {0};
    jfs_fio_path_t   dir_path = {0};
    jfs_fw_dir_t     dir = {0};
    DIR             *sys_dir = NULL;

    err = fw_file_vector_init(&file_vec);
    GOTO_IF_ERR(err, cleanup);

    err = fw_path_vector_pop(&state->path_vec, &dir_path);
    GOTO_IF_ERR(err, cleanup);

    err = jfs_err_opendir(&sys_dir, dir_path.str);
    if (err == JFS_ERR_ACCESS) err = JFS_FW_ERR_SKIP;
    GOTO_IF_ERR(err, cleanup);

    err = fw_scan_dir(sys_dir, &file_vec);
    GOTO_IF_ERR(err, cleanup);

    err = fw_push_dir_paths(&state->path_vec, &file_vec, &dir_path);
    GOTO_IF_ERR(err, cleanup);

    err = fw_dir_init(&dir, &file_vec, &dir_path);
    GOTO_IF_ERR(err, cleanup);

    err = fw_dir_vector_push(&state->dir_vec, &dir);
    GOTO_IF_ERR(err, cleanup);

    *done_flag = state->path_vec.count == 0;
    closedir(sys_dir);

    return JFS_OK;
cleanup:
    if (sys_dir != NULL) closedir(sys_dir);
    fw_file_vector_free(&file_vec);
    jfs_fio_path_free(&dir_path);
    jfs_fw_dir_free(&dir);
    RETURN_ERR(err);
}

jfs_error_t jfs_fw_record_init(jfs_fw_record_t *record_init, jfs_fw_state_t **state_give) {
    jfs_fw_state_t *state = *state_give;
    FAIL_IF(state->path_vec.count > 0, JFS_FW_ERR_STATE);

    size_t        new_dir_count = state->dir_vec.count;
    jfs_fw_dir_t *new_dir_array;

    CHECK_ERR(fw_dir_vector_to_array(&new_dir_array, &state->dir_vec));
    jfs_fw_state_destroy(state_give);

    record_init->dir_array = new_dir_array;
    record_init->dir_count = new_dir_count;

    return JFS_OK;
}

void jfs_fw_record_free(jfs_fw_record_t *record_free) {
    if (record_free->dir_array != NULL) {
        for (size_t i = 0; i < record_free->dir_count; i++) {
            jfs_fw_dir_free(&record_free->dir_array[i]);
        }

        free(record_free->dir_array);
    }

    memset(record_free, 0, sizeof(*record_free));
}

static jfs_error_t fw_map_dirent_type(unsigned char ent_type, jfs_fw_types_t *fw_type) {
    switch (ent_type) {
        case DT_REG:     *fw_type = JFS_FW_REG; break;
        case DT_DIR:     *fw_type = JFS_FW_DIR; break;
        case DT_UNKNOWN: RETURN_ERR(JFS_FW_ERR_UNKNOWN);
        default:         RETURN_ERR(JFS_FW_ERR_UNSUPPORTED);
    }

    return JFS_OK;
}

static jfs_error_t fw_file_init(jfs_fw_file_t *file_init, const struct dirent *ent) {
    jfs_fw_types_t new_type;
    jfs_fio_name_t new_name = {0};
    CHECK_ERR(fw_map_dirent_type(ent->d_type, &new_type));
    CHECK_ERR(jfs_fio_name_init(&new_name, ent->d_name));

    jfs_fio_name_transfer(&file_init->name, &new_name);
    file_init->inode = ent->d_ino;
    file_init->type = new_type;

    return JFS_OK;
}

static jfs_error_t fw_dir_init(jfs_fw_dir_t *dir_init, fw_file_vector_t *vec_free, jfs_fio_path_t *path_free) {
    jfs_fw_file_t *new_files = NULL;
    size_t         new_count = vec_free->count;

    if (vec_free->count > 0) {
        CHECK_ERR(fw_file_vector_to_array(&new_files, vec_free));
    } else {
        fw_file_vector_free(vec_free);
    }

    dir_init->file_count = new_count;
    dir_init->files = new_files;
    jfs_fio_path_transfer(&dir_init->path, path_free);

    return JFS_OK;
}

static jfs_error_t fw_path_vector_init(fw_path_vector_t *vec_init) {
    jfs_fio_path_t *new_path_array = malloc(sizeof(*new_path_array) * FW_PATH_VECTOR_DEFAULT_CAPACITY);
    FAIL_IF(new_path_array == NULL, JFS_ERR_RESOURCE);

    vec_init->path_array = new_path_array;
    vec_init->capacity = FW_PATH_VECTOR_DEFAULT_CAPACITY;
    vec_init->count = 0;

    return JFS_OK;
}

static void fw_path_vector_free(fw_path_vector_t *vec_free) {
    if (vec_free->path_array != NULL) {
        for (size_t i = 0; i < vec_free->count; i++) {
            jfs_fio_path_free(&vec_free->path_array[i]);
        }

        free(vec_free->path_array);
    }

    memset(vec_free, 0, sizeof(*vec_free));
}

static jfs_error_t fw_path_vector_push(fw_path_vector_t *vec, jfs_fio_path_t *path_free) {
    if (vec->count >= vec->capacity) {
        const size_t new_capacity = vec->capacity * 2;

        jfs_fio_path_t *new_path_array = realloc(vec->path_array, sizeof(*new_path_array) * new_capacity);
        FAIL_IF(new_path_array == NULL, JFS_ERR_RESOURCE);

        vec->path_array = new_path_array;
        vec->capacity = new_capacity;
    }

    jfs_fio_path_transfer(&vec->path_array[vec->count], path_free);
    vec->count += 1;

    return JFS_OK;
}

static jfs_error_t fw_path_vector_pop(fw_path_vector_t *vec, jfs_fio_path_t *path_init) {
    FAIL_IF(vec->count == 0, JFS_ERR_EMPTY);

    vec->count -= 1;
    jfs_fio_path_transfer(path_init, &vec->path_array[vec->count]);

    return JFS_OK;
}

static jfs_error_t fw_file_vector_init(fw_file_vector_t *vec_init) {
    jfs_fw_file_t *new_file_array = malloc(sizeof(*new_file_array) * FW_FILE_VECTOR_DEFAULT_CAPACITY);
    FAIL_IF(new_file_array == NULL, JFS_ERR_RESOURCE);

    vec_init->file_array = new_file_array;
    vec_init->capacity = FW_FILE_VECTOR_DEFAULT_CAPACITY;
    vec_init->count = 0;

    return JFS_OK;
}

static void fw_file_vector_free(fw_file_vector_t *vec_free) {
    if (vec_free->file_array != NULL) {
        for (size_t i = 0; i < vec_free->count; i++) {
            jfs_fw_file_free(&vec_free->file_array[i]);
        }

        free(vec_free->file_array);
    }

    memset(vec_free, 0, sizeof(*vec_free));
}

static jfs_error_t fw_file_vector_push(fw_file_vector_t *vec, jfs_fw_file_t *file_free) {
    if (vec->count >= vec->capacity) {
        const size_t new_capacity = vec->capacity * 2;

        jfs_fw_file_t *new_file_array = realloc(vec->file_array, sizeof(*new_file_array) * new_capacity);
        FAIL_IF(new_file_array == NULL, JFS_ERR_RESOURCE);

        vec->file_array = new_file_array;
        vec->capacity = new_capacity;
    }

    jfs_fw_file_transfer(&vec->file_array[vec->count], file_free);
    vec->count += 1;

    return JFS_OK;
}

static jfs_error_t fw_file_vector_to_array(jfs_fw_file_t **file_array_take, fw_file_vector_t *vec_free) {
    FAIL_IF(vec_free->count == 0, JFS_ERR_EMPTY);

    jfs_fw_file_t *new_file_array = realloc(vec_free->file_array, sizeof(*new_file_array) * vec_free->count);
    FAIL_IF(new_file_array == NULL, JFS_ERR_RESOURCE);

    *file_array_take = new_file_array;
    memset(vec_free, 0, sizeof(*vec_free));

    return JFS_OK;
}

static jfs_error_t fw_dir_vector_init(fw_dir_vector_t *vec_init) {
    jfs_fw_dir_t *new_dir_array = malloc(sizeof(*new_dir_array) * FW_DIR_VECTOR_DEFAULT_CAPACITY);
    FAIL_IF(new_dir_array == NULL, JFS_ERR_RESOURCE);

    vec_init->dir_array = new_dir_array;
    vec_init->capacity = FW_DIR_VECTOR_DEFAULT_CAPACITY;
    vec_init->count = 0;

    return JFS_OK;
}

static void fw_dir_vector_free(fw_dir_vector_t *vec_free) {
    if (vec_free->dir_array != NULL) {
        for (size_t i = 0; i < vec_free->count; i++) {
            jfs_fw_dir_free(&vec_free->dir_array[i]);
        }

        free(vec_free->dir_array);
    }

    memset(vec_free, 0, sizeof(*vec_free));
}

static jfs_error_t fw_dir_vector_push(fw_dir_vector_t *vec, jfs_fw_dir_t *dir_free) {
    if (vec->count >= vec->capacity) {
        const size_t new_capacity = vec->capacity * 2;

        jfs_fw_dir_t *new_dir_array = realloc(vec->dir_array, sizeof(*new_dir_array) * new_capacity);
        FAIL_IF(new_dir_array == NULL, JFS_ERR_RESOURCE);

        vec->dir_array = new_dir_array;
        vec->capacity = new_capacity;
    }

    jfs_fw_dir_transfer(&vec->dir_array[vec->count], dir_free);
    vec->count += 1;

    return JFS_OK;
}

static jfs_error_t fw_dir_vector_to_array(jfs_fw_dir_t **dir_array_take, fw_dir_vector_t *vec_free) {
    FAIL_IF(vec_free->count == 0, JFS_ERR_EMPTY);

    jfs_fw_dir_t *new_dir_array = realloc(vec_free->dir_array, sizeof(*new_dir_array) * vec_free->count);
    FAIL_IF(new_dir_array == NULL, JFS_ERR_RESOURCE);

    *dir_array_take = new_dir_array;
    memset(vec_free, 0, sizeof(*vec_free));

    return JFS_OK;
}

static jfs_error_t fw_scan_dir(DIR *dir, fw_file_vector_t *vec) {
    struct dirent *ent = NULL;

    errno = 0;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        jfs_error_t err = fw_handle_dirent(ent, vec);
        if (err == JFS_FW_ERR_UNSUPPORTED) {
            JFS_RES(err);
            continue;
        }
        CHECK_ERR(err);
    }

    FAIL_IF(errno != 0, JFS_ERR_BAD_FD);

    return JFS_OK;
}

static jfs_error_t fw_handle_dirent(const struct dirent *ent, fw_file_vector_t *vec) {
    jfs_fw_file_t file = {0};
    CHECK_ERR(fw_file_init(&file, ent));

    jfs_error_t err = fw_file_vector_push(vec, &file);
    if (err != JFS_OK) {
        jfs_fw_file_free(&file);
        RETURN_ERR(err);
    }

    return JFS_OK;
}

static jfs_error_t fw_push_dir_paths(fw_path_vector_t *path_vec, fw_file_vector_t *file_vec, const jfs_fio_path_t *dir_path) {
    jfs_fio_path_t     push_path = {0};
    jfs_fio_path_buf_t buf = {0};

    for (size_t i = 0; i < file_vec->count; i++) {
        if (file_vec->file_array[i].type == JFS_FW_REG) continue;
        CHECK_ERR(jfs_fio_path_buf_compose(&buf, dir_path, &(file_vec->file_array[i].name)));
        CHECK_ERR(jfs_fio_path_init(&push_path, buf.data));
        if (fw_path_vector_push(path_vec, &push_path) != JFS_OK) {
            jfs_fio_path_free(&push_path);
        }
    }

    return JFS_OK;
}
