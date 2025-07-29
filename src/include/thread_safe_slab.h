#ifndef JFS_THREAD_SAFE_SLAB_H
#define JFS_THREAD_SAFE_SLAB_H

#include <stdatomic.h>

#define SLAB_SIZE 4096 // 4KB page size

typedef struct jfs_tss_slab  jfs_tss_slab_t;
typedef struct jfs_tss_cache jfs_tss_cache;

struct jfs_tss_slab {
    void         *memory_block;
    atomic_size_t used_count;
};

struct jfs_tss_cache {
    void *free_list;
    void *block_list;
    size_t obj_size;
};

#endif
