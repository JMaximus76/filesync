#ifndef FILESYNC_FILE_TOOLS_H
#define FILESYNC_FILE_TOOLS_H

#include <stdlib.h>
#include <limits.h>

struct ft_dir_id {
    ino_t inode;
    char name[NAME_MAX];
};

struct ft_child_dirs;

struct ft_dir;


void ft_child_dirs_destroy(struct ft_child_dirs **cdirs);

/**
 * @brief A safe wraper for writing to files
 *
 * Writes size bytes from buf into fd. Will perform multiple writes
 * if partial write is detected, and handels EINTER. The return value is
 * the numbe of bytes written. If this number is less than size an error
 * occurred and errno should be checked. If -1 is returned no bytes were
 * written.
 *
 * The function will fail and could segfault if buf is not larger than or
 * equal to size. This function is only meant for normal files and not other
 * types of fd.
 *
 * Errno: EINVAL if size is greater than SSIZE_MAX or equal to 0
 *        write()
 *
 * @param fd File descriptor to write to.
 * @param buf Buffer to write from.
 * @param size Number of bytes to write from buf.
 * @return Number of bytes writen
 *         -1 on error
 */
ssize_t ft_write(int fd, const void *buf, size_t size);


/**
 * @brief A safe wraper for reading files
 *
 * Reads size bytes from fd into buf using read(). This function is only
 * meant for normal files and not other fd. Protects agains EINTER. Returns
 * number of bytes read, and can be less than size.
 *
 * The function will fail and could segfault if buf is not larger than or
 * equal to size.
 *
 * Errno: read()
 *
 * @param fd File descriptor to read from.
 * @param buf Buffer to write into.
 * @param size Number of bytes to read from buf.
 * @return Success is number of bytes read.
 *         End of file is 0.
 *         Failure is -1.
 */
ssize_t ft_read(int fd, void *buf, size_t size);

#endif
