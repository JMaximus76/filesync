#ifndef JFS_THREAD_SAFE_SLAB_H
#define JFS_THREAD_SAFE_SLAB_H

#include "error.h"
#include <pthread.h>
#include <stdbool.h>

#define TSS_PAGE_SIZE    ((size_t) 4096) // 4 kb
#define TSS_MIN_OBJ_SIZE sizeof(jfs_tss_obj_t *)

typedef struct jfs_tss_obj              jfs_tss_obj_t;
typedef struct jfs_tss_allocator        jfs_tss_allocator_t;
typedef struct jfs_tss_allocator_config jfs_tss_allocator_config_t;
typedef struct jfs_tss_cache            jfs_tss_cache_t;

struct jfs_tss_allocator_config {
    size_t   obj_size;
    size_t   obj_align;
    size_t   retire_threshold;  // can zero to mean there is no retirement
    float    percent_to_retire; // can zero for default
    uint32_t pages_per_slab;    // can zero for default
    uint32_t cache_cap;         // can zero for default
    uint32_t cache_acquire;     // can zero for default
    uint32_t cache_release;     // can zero for default
};

struct jfs_tss_cache {
    jfs_tss_allocator_t *alloc;
    jfs_tss_obj_t       *free_list_head;
    uint32_t             free_list_count;
};

jfs_tss_allocator_t *jfs_tss_allocator_create(const jfs_tss_allocator_config_t *config, jfs_err_t *err) WUR;
void  jfs_tss_allocator_destroy(jfs_tss_allocator_t *alloc_move); // MUST ENSURE that no threads can use the allocator when this is called
void  jfs_tss_allocator_explicit_retire(jfs_tss_allocator_t *alloc);
void  jfs_tss_cache_init(jfs_tss_cache_t *cache_init, jfs_tss_allocator_t *alloc);
void  jfs_tss_cache_refresh(jfs_tss_cache_t *cache, jfs_err_t *err);
void  jfs_tss_cache_full_release(jfs_tss_cache_t *cache);
void *jfs_tss_alloc(jfs_tss_cache_t *cache, jfs_err_t *err) WUR;
void  jfs_tss_free(jfs_tss_cache_t *cache, void *free);

#endif
