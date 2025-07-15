#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include "file_tools.h"

struct ft_child_dirs_node {
    struct ft_child_dirs_node *next;
    char *path_name;
};

struct ft_child_dirs {
    struct ft_child_dirs_node *head;
};

struct ft_dir_node {
    struct ft_dir_node *next;
    struct ft_dir_id id;
};

struct ft_dir {
    char path_name[PATH_MAX];
    struct ft_dir_node *head;
    size_t list_size;
};



struct ft_dir* ft_read_dir(const char *path) {
    DIR *d = opendir(path);
    if (d == NULL) return NULL;



    struct dirent *ent;
    errno = 0;
    while ((ent = readdir(d)) != NULL) {
    }

    if (errno != 0) {
        return -1;
    }

    closedir(d);
    return 0;
}

void ft_dir_destroy(struct ft_dir **dir_ptr) {
    if (dir_ptr == NULL || *dir_ptr == NULL) return;

    struct ft_dir *dir = *dir_ptr;

    struct ft_dir_node *node = dir->head;
    while (node != NULL) {
        struct ft_dir_node *temp = node->next;
        free(node);
        node = temp;
    }

    if (dir->child_dir_paths != NULL) {
        for (size_t i = 0; i < dir->dir_paths_count; i++) {
            free(dir->child_dir_paths[i]);
        }
    }

    free(dir->child_dir_paths);
    free(dir);
    *dir_ptr = NULL;
}

void ft_child_dirs_destroy(struct ft_child_dirs **cdirs) {
    if (cdirs == NULL || *cdirs == NULL) return;

    for (size_t i = 0; i < (*cdirs)->count; i++) {
        free((*cdirs)->dirs[i]);
    }

    free(*cdirs);
    *cdirs = NULL;
}


static struct ft_dir* ft_dir_create() {
    struct ft_dir *dir = malloc(sizeof(*dir));
    if (dir == NULL) return NULL;
    dir->child_dirs = malloc(sizeof(*(dir->child_dirs)));
    if (dir->child_dirs == NULL) {
        free(dir);
        return NULL;
    }

    dir->head = NULL;
    dir->curr = NULL;
    return dir;
}

static int ft_dir_add(struct ft_dir *dir, const struct dirent *ent) {
    struct ft_dir_node *node = malloc(sizeof(*node));
    if (node == NULL) return -1;

    node->id.inode = ent->d_ino;
    strcpy(node->id.name, ent->d_name);

    node->next = dir->head;
    dir->head = node;

    return 0;
}

static int ft_child_dirs_add(struct ft_child_dirs *cdirs, const char *dir_path) {
    if (dir_path == NULL) {
        errno = EINVAL;
        return -1;
    }

    char *name = malloc(strlen(dir_path) + 1);
    if (name == NULL) return -1;

    strcpy(name, dir_path);

    cdirs->dirs[cdirs->count] = 




ssize_t ft_write(int fd, const void *buf, size_t size) {
    if (size > SSIZE_MAX || size == 0) {
        errno = EINVAL;
        return -1;
    }

    const uint8_t *write_buf = (uint8_t*) buf;

    size_t total_written = 0;
    while (total_written < size) {
        const ssize_t w = write(fd, write_buf + total_written, size - total_written);

        if (w == -1) {
            if (errno == EINTR) continue;
            return (total_written == 0) ? -1 : (ssize_t) total_written;
        }

        total_written += (size_t) w;
    }

    return (ssize_t) total_written;
}

ssize_t ft_read(int fd, void *buf, size_t size) {
    ssize_t r;
    do {
        r = read(fd, buf, size);
    } while (r == -1 && errno == EINTR);

    return r;
}

