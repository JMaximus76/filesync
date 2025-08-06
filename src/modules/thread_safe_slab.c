#include "thread_safe_slab.h"
#include "error.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_PAGE_PER_SLAB     8
#define DEFAULT_CACHE_CAP         64
#define DEFAULT_CACHE_ACQUIRE     32
#define DEFAULT_CACHE_RELEASE     32
#define DEFAULT_PERCENT_TO_RETIRE 0.1F
#define MIN_OBJ_SIZE              sizeof(jfs_tss_obj_t)

typedef struct tss_batch        tss_batch_t;
typedef struct tss_slab_config  tss_slab_config_t;
typedef struct tss_cache_config tss_cache_config_t;
typedef struct tss_slab         tss_slab_t;

struct jfs_tss_obj {
    jfs_tss_obj_t *next;
};

struct tss_batch {
    jfs_tss_obj_t *head;
    jfs_tss_obj_t *tail;
    uint32_t       current_count;
    uint32_t       target_count;
};

struct tss_slab_config {
    size_t    obj_align;
    size_t    padded_obj_size;
    size_t    slab_size;
    size_t    obj_per_slab;
    size_t    retire_threshold;
    uintptr_t slab_offset;
};

struct tss_cache_config {
    uint32_t cache_cap;
    uint32_t cache_acquire;
    uint32_t cache_release;
};

struct tss_slab {
    size_t         used_count;
    jfs_tss_obj_t *free_list_head;
    tss_slab_t    *next;
    bool           retired_flag;
};

struct jfs_tss_allocator {
    pthread_mutex_t    lock;
    tss_slab_t        *active_list_head;
    size_t             active_list_count;
    size_t             used_obj_count;
    tss_slab_config_t  slab_conf;
    tss_cache_config_t cache_conf;
    float              percent_to_retire;
};

static void  tss_alloc_slow_path(jfs_tss_cache_t *cache, jfs_err_t *err);
static void  tss_free_slow_path(jfs_tss_cache_t *cache);
static void  tss_slab_config_init(tss_slab_config_t *config_init, const jfs_tss_allocator_config_t *alloc_conf);
static void  tss_cache_config_init(tss_cache_config_t *config_init, const jfs_tss_allocator_config_t *alloc_conf);
static void *tss_aligned_mmap(const tss_slab_config_t *conf, jfs_err_t *err) WUR;

static tss_slab_t    *tss_slab_create(const tss_slab_config_t *conf, jfs_err_t *err) WUR;
static void           tss_slab_destroy(tss_slab_t *slab_move, const tss_slab_config_t *conf);
static jfs_tss_obj_t *tss_slab_link_block(void *raw_block, const tss_slab_config_t *conf) WUR;

static void        tss_batch_init(tss_batch_t *batch_init, uint32_t target_count);
static uint32_t    tss_batch_load(tss_batch_t *batch, jfs_tss_obj_t **list_head) WUR;
static uint32_t    tss_batch_unload(tss_batch_t *batch, jfs_tss_obj_t **list_head) WUR;
static inline bool tss_batch_met_target(const tss_batch_t *batch) WUR;

static void               tss_allocator_batch_acquire(jfs_tss_allocator_t *alloc, tss_batch_t *batch, jfs_err_t *err);
static void               tss_allocator_batch_release(jfs_tss_allocator_t *alloc, tss_batch_t *batch);
static void               tss_allocator_release_obj(jfs_tss_allocator_t *alloc, jfs_tss_obj_t *obj_free);
static inline tss_slab_t *tss_allocator_find_slab(const jfs_tss_allocator_t *alloc, const jfs_tss_obj_t *obj) WUR;
static void               tss_allocator_add_slab(jfs_tss_allocator_t *alloc, jfs_err_t *err);
static bool               tss_allocator_ready_for_retire(const jfs_tss_allocator_t *alloc);
static void               tss_allocator_retire_slabs(jfs_tss_allocator_t *alloc);

jfs_tss_allocator_t *jfs_tss_allocator_create(const jfs_tss_allocator_config_t *config, jfs_err_t *err) {
    assert(config != NULL);
    assert(err != NULL);

    jfs_tss_allocator_t *alloc = jfs_malloc(sizeof(*alloc), err);
    NULL_CHECK_ERR;

    int ret = pthread_mutex_init(&alloc->lock, NULL);
    assert(ret == 0 && "glibc shouldn't fail");
    (void) ret;

    alloc->active_list_head = NULL;
    alloc->active_list_count = 0;
    alloc->used_obj_count = 0;
    tss_slab_config_init(&alloc->slab_conf, config);
    tss_cache_config_init(&alloc->cache_conf, config);
    if (config->percent_to_retire > 1.0F || config->percent_to_retire <= 0.0F) {
        alloc->percent_to_retire = DEFAULT_PERCENT_TO_RETIRE;
    } else {
        alloc->percent_to_retire = config->percent_to_retire;
    }

    return alloc;
}

void jfs_tss_allocator_destroy(jfs_tss_allocator_t *alloc_move) {
    pthread_mutex_lock(&alloc_move->lock);

    while (alloc_move->active_list_head != NULL) {
        tss_slab_t *temp = alloc_move->active_list_head;
        alloc_move->active_list_head = alloc_move->active_list_head->next;
        tss_slab_destroy(temp, &alloc_move->slab_conf);
    }
    assert(alloc_move->active_list_head == NULL);

    pthread_mutex_unlock(&alloc_move->lock);
    int ret = pthread_mutex_destroy(&alloc_move->lock);
    // my code should ensure that any thread that would want to lock this mutex is joined by the time its destroying stuff like this
    assert(ret == 0 && "mutex was busy when trying to destroy an allocator");
    (void) ret;

    free(alloc_move);
}

void jfs_tss_allocator_explicit_retire(jfs_tss_allocator_t *alloc) {
    assert(alloc != NULL);
    pthread_mutex_lock(&alloc->lock);
    tss_allocator_retire_slabs(alloc);
    pthread_mutex_unlock(&alloc->lock);
}

void jfs_tss_cache_init(jfs_tss_cache_t *cache_init, jfs_tss_allocator_t *alloc) {
    memset(cache_init, 0, sizeof(*cache_init));
    cache_init->alloc = alloc;
}

void jfs_tss_cache_refresh(jfs_tss_cache_t *cache, jfs_err_t *err) {
    assert(cache != NULL);
    assert(err != NULL);

    tss_batch_t batch = {0};
    tss_batch_init(&batch, cache->free_list_count);
    uint32_t total_loaded = tss_batch_load(&batch, &cache->free_list_head);
    assert(total_loaded == cache->free_list_count);
    (void) total_loaded;

    pthread_mutex_lock(&cache->alloc->lock);

    tss_allocator_batch_release(cache->alloc, &batch);
    assert(batch.current_count == 0);

    tss_batch_init(&batch, cache->alloc->cache_conf.cache_acquire);
    tss_allocator_batch_acquire(cache->alloc, &batch, err);

    if (*err != JFS_OK) {
        tss_allocator_batch_release(cache->alloc, &batch);
        pthread_mutex_unlock(&cache->alloc->lock);
        VOID_RETURN_ERR;
    }

    pthread_mutex_unlock(&cache->alloc->lock);

    assert(tss_batch_met_target(&batch)); // this is fine since the function is just a predicate
    const uint32_t unload_count = tss_batch_unload(&batch, &cache->free_list_head);
    cache->free_list_count += unload_count;
}

void jfs_tss_cache_full_release(jfs_tss_cache_t *cache) {
    assert(cache != NULL);

    if (cache->free_list_count == 0) return; // nothing to release

    tss_batch_t batch = {0};
    tss_batch_init(&batch, cache->free_list_count);
    uint32_t load_count = tss_batch_load(&batch, &cache->free_list_head);
    cache->free_list_count -= load_count;
    assert(cache->free_list_head == NULL);
    assert(cache->free_list_count == 0);

    pthread_mutex_lock(&cache->alloc->lock);
    tss_allocator_batch_release(cache->alloc, &batch);
    pthread_mutex_unlock(&cache->alloc->lock);
}

void *jfs_tss_alloc(jfs_tss_cache_t *cache, jfs_err_t *err) {
    if (cache->free_list_count == 0) {
        // slow path, need to get more objs from allocator
        tss_alloc_slow_path(cache, err);
        NULL_CHECK_ERR;
    }

    void *ret = cache->free_list_head;
    cache->free_list_head = cache->free_list_head->next;
    cache->free_list_count -= 1;
    return ret;
}

void jfs_tss_free(jfs_tss_cache_t *cache, void *free) {
    if (free == NULL) return;
    jfs_tss_obj_t *obj = free;

    obj->next = cache->free_list_head;
    cache->free_list_head = obj;
    cache->free_list_count += 1;

    if (cache->free_list_count >= cache->alloc->cache_conf.cache_cap) {
        // slow path, we release unused free objs
        tss_free_slow_path(cache);
    }
}

static void tss_alloc_slow_path(jfs_tss_cache_t *cache, jfs_err_t *err) {
    tss_batch_t batch = {0};
    tss_batch_init(&batch, cache->alloc->cache_conf.cache_acquire);

    pthread_mutex_lock(&cache->alloc->lock);
    tss_allocator_batch_acquire(cache->alloc, &batch, err);

    if (*err != JFS_OK) {
        tss_allocator_batch_release(cache->alloc, &batch);
        pthread_mutex_unlock(&cache->alloc->lock);
        VOID_RETURN_ERR;
    }

    if (tss_allocator_ready_for_retire(cache->alloc)) {
        tss_allocator_retire_slabs(cache->alloc);
    }

    pthread_mutex_unlock(&cache->alloc->lock);

    assert(tss_batch_met_target(&batch)); // this is fine since the function is just a predicate
    const uint32_t unload_count = tss_batch_unload(&batch, &cache->free_list_head);
    cache->free_list_count += unload_count;
}

static void tss_free_slow_path(jfs_tss_cache_t *cache) {
    tss_batch_t batch = {0};

    tss_batch_init(&batch, cache->alloc->cache_conf.cache_release);
    uint32_t load_count = tss_batch_load(&batch, &cache->free_list_head);
    assert(tss_batch_met_target(&batch));
    cache->free_list_count -= load_count;

    pthread_mutex_lock(&cache->alloc->lock);
    tss_allocator_batch_release(cache->alloc, &batch);
    pthread_mutex_unlock(&cache->alloc->lock);
}

static void tss_slab_config_init(tss_slab_config_t *config_init, const jfs_tss_allocator_config_t *alloc_config) {
    config_init->obj_align = alloc_config->obj_align;
    assert(config_init->obj_align > 0);

    size_t padded_size = alloc_config->obj_size > TSS_MIN_OBJ_SIZE ? alloc_config->obj_size : MIN_OBJ_SIZE;
    config_init->padded_obj_size = (padded_size + config_init->obj_align - 1) & ~(config_init->obj_align - 1);

    const uint32_t pages_per_slab = alloc_config->pages_per_slab ? alloc_config->pages_per_slab : DEFAULT_PAGE_PER_SLAB;
    config_init->slab_size = pages_per_slab * TSS_PAGE_SIZE;

    config_init->slab_offset = (sizeof(tss_slab_t) + alloc_config->obj_align - 1) & ~(alloc_config->obj_align - 1);
    assert(config_init->slab_offset % alloc_config->obj_align == 0);
    assert((config_init->slab_size - config_init->slab_offset) % config_init->obj_align == 0);

    config_init->obj_per_slab = (config_init->slab_size - config_init->slab_offset) / config_init->padded_obj_size;
    assert(config_init->obj_per_slab > 0);

    config_init->retire_threshold = alloc_config->retire_threshold;
    assert(config_init->obj_per_slab > config_init->retire_threshold);
}

static void tss_cache_config_init(tss_cache_config_t *config_init, const jfs_tss_allocator_config_t *alloc_conf) {
    config_init->cache_cap = alloc_conf->cache_cap ? alloc_conf->cache_cap : DEFAULT_CACHE_CAP;

    config_init->cache_acquire = alloc_conf->cache_acquire ? alloc_conf->cache_acquire : DEFAULT_CACHE_ACQUIRE;
    assert(config_init->cache_cap >= config_init->cache_acquire);

    config_init->cache_release = alloc_conf->cache_release ? alloc_conf->cache_release : DEFAULT_CACHE_RELEASE;
    assert(config_init->cache_cap >= config_init->cache_release);
}

static void *tss_aligned_mmap(const tss_slab_config_t *conf, jfs_err_t *err) { // NOLINT(readability-function-cognitive-complexity)
    assert(conf->slab_size > 0);
    assert(err != NULL);

    void *const raw_block = jfs_mmap(NULL, conf->slab_size * 2, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0, err);
    NULL_CHECK_ERR;

    const uintptr_t raw_addr = (uintptr_t) raw_block;
    const uintptr_t aligned_addr = (raw_addr + conf->slab_size - 1) & ~(conf->slab_size - 1);
    void *const     aligned_block = (void *) aligned_addr; // NOLINT
    const size_t    leading_trim = aligned_addr - raw_addr;
    const size_t    trailing_trim = (raw_addr + 2 * conf->slab_size) - (aligned_addr + conf->slab_size);

    assert(raw_addr % TSS_PAGE_SIZE == 0);
    assert(aligned_addr % conf->slab_size == 0);
    assert(leading_trim % TSS_PAGE_SIZE == 0);
    assert(trailing_trim % TSS_PAGE_SIZE == 0);

    // these munmap are assuming:
    //   - both addresses are within the proceses address space
    //   - len is > 0 (the ifs check this)
    //   - the addr's are multiples of page_size which the asserts above check
    if (leading_trim > 0) {
        int ret = munmap(raw_block, leading_trim); // NOLINT
        assert(ret == 0 && "munmap shouldn't be able to fail here");
    }

    if (trailing_trim > 0) {
        int ret = munmap((void *) aligned_addr + conf->slab_size, trailing_trim); // NOLINT
        assert(ret == 0 && "munmap shouldn't be able to fail here");
    }

    return aligned_block;
}

static tss_slab_t *tss_slab_create(const tss_slab_config_t *conf, jfs_err_t *err) {
    assert(err != NULL);

    void *raw_block = tss_aligned_mmap(conf, err);
    NULL_CHECK_ERR;

    jfs_tss_obj_t *const free_list = tss_slab_link_block(raw_block, conf);

    tss_slab_t *slab = raw_block;
    slab->free_list_head = free_list;
    slab->next = NULL;
    slab->used_count = 0;
    slab->retired_flag = false;

    return slab;
}

static void tss_slab_destroy(tss_slab_t *slab_move, const tss_slab_config_t *conf) {
    if (slab_move == NULL) return;

    assert(conf->slab_size > 0);
    int ret = munmap(slab_move, conf->slab_size);
    assert(ret == 0 && "obj_memory erroneously modified");
    (void) ret;
}

static jfs_tss_obj_t *tss_slab_link_block(void *raw_block, const tss_slab_config_t *conf) {
    assert(raw_block != NULL);
    assert(conf != NULL);
    assert(TSS_PAGE_SIZE == sysconf(_SC_PAGESIZE));

    uint8_t *bytes = raw_block;
    bytes += conf->slab_offset;

    jfs_tss_obj_t *list = NULL;
    for (size_t i = 0; i < conf->obj_per_slab; i++) {
        jfs_tss_obj_t *obj = (jfs_tss_obj_t *) (bytes + (i * conf->padded_obj_size));
        obj->next = list;
        list = obj;
    }

    return list;
}

static void tss_batch_init(tss_batch_t *batch_init, uint32_t target_count) {
    batch_init->target_count = target_count;
    batch_init->head = NULL;
    batch_init->tail = NULL;
    batch_init->current_count = 0;
}

static uint32_t tss_batch_load(tss_batch_t *batch, jfs_tss_obj_t **list_head) {
    assert(batch != NULL);
    assert(list_head != NULL);
    assert(batch->target_count > batch->current_count); // target was reached already

    if (*list_head == NULL) return 0; // empty list

    uint32_t       total = 1;
    uint32_t       target = batch->target_count - batch->current_count;
    jfs_tss_obj_t *list_tail = *list_head;

    while (total < target && list_tail->next != NULL) {
        list_tail = list_tail->next;
        total += 1;
    }

    if (batch->current_count == 0) {
        assert(batch->head == NULL);
        assert(batch->tail == NULL);
        batch->head = *list_head;
    } else {
        assert(batch->tail->next == NULL);
        batch->tail->next = *list_head;
    }

    batch->tail = list_tail;
    *list_head = list_tail->next;
    list_tail->next = NULL;
    batch->current_count += total;
    return total;
}

static uint32_t tss_batch_unload(tss_batch_t *batch, jfs_tss_obj_t **list_head) {
    assert(batch != NULL);
    assert(list_head != NULL);

    if (batch->target_count == 0) return 0;

    assert(batch->head != NULL);
    assert(batch->tail != NULL);
    assert(batch->tail->next == NULL);

    batch->tail->next = *list_head;
    *list_head = batch->head;

    uint32_t unload_count = batch->current_count;
    batch->current_count = 0;
    batch->head = NULL;
    batch->tail = NULL;
    return unload_count;
}

static inline bool tss_batch_met_target(const tss_batch_t *batch) {
    return batch->current_count >= batch->target_count;
}

static void tss_allocator_batch_acquire(jfs_tss_allocator_t *alloc, tss_batch_t *batch, jfs_err_t *err) {
    assert(batch != NULL);
    assert(alloc != NULL);
    assert(batch->target_count > 0);
    assert(batch->current_count == 0);
    assert(batch->head == NULL);
    assert(batch->tail == NULL);

    const size_t obj_remaining = (alloc->slab_conf.obj_per_slab * alloc->active_list_count) - alloc->used_obj_count;
    if (obj_remaining < batch->target_count) {
        const size_t obj_needed = batch->target_count - obj_remaining;
        const size_t number_of_slabs = (obj_needed / alloc->slab_conf.obj_per_slab) + 1;


        for (size_t i = 0; i < number_of_slabs; i++) {
            tss_allocator_add_slab(alloc, err);
            VOID_CHECK_ERR;
        }
    }

    tss_slab_t *current_slab = alloc->active_list_head;
    while (batch->current_count < batch->target_count && current_slab != NULL) {
        const uint32_t load_count = tss_batch_load(batch, &current_slab->free_list_head);
        current_slab->used_count += load_count;
        alloc->used_obj_count += load_count;
        current_slab = current_slab->next;
    }

    assert(tss_batch_met_target(batch));
}

static void tss_allocator_batch_release(jfs_tss_allocator_t *alloc, tss_batch_t *batch) {
    assert(batch != NULL);

    jfs_tss_obj_t *current_obj = batch->head;
    while (current_obj != NULL) {
        jfs_tss_obj_t *temp = current_obj;
        current_obj = current_obj->next;
        tss_allocator_release_obj(alloc, temp);
        // i could try to do batch releases but then i would need to sort through to find
        // the objs from the same slab. seems like too much work.
    }

    batch->current_count = 0;
    batch->head = NULL;
    batch->tail = NULL;
}

static void tss_allocator_release_obj(jfs_tss_allocator_t *alloc, jfs_tss_obj_t *obj_free) {
    tss_slab_t *slab = tss_allocator_find_slab(alloc, obj_free);
    obj_free->next = slab->free_list_head;
    slab->free_list_head = obj_free;
    slab->used_count -= 1;
    alloc->used_obj_count -= 1;

    // SUPER important, this is where we destroy the slab when its empty.
    // nowhere else is there a pointer to this slab at this point so we
    // need to do it here
    if (slab->used_count == 0 && slab->retired_flag) {
        tss_slab_destroy(slab, &alloc->slab_conf);
    }
}

static inline tss_slab_t *tss_allocator_find_slab(const jfs_tss_allocator_t *alloc, const jfs_tss_obj_t *obj) {
    return (tss_slab_t *) (((uintptr_t) obj) & ~(alloc->slab_conf.slab_size - 1)); // NOLINT
}

static void tss_allocator_add_slab(jfs_tss_allocator_t *alloc, jfs_err_t *err) {
    assert(alloc != NULL);
    assert(err != NULL);

    tss_slab_t *new_slab = tss_slab_create(&alloc->slab_conf, err);
    VOID_CHECK_ERR;

    new_slab->next = alloc->active_list_head;
    alloc->active_list_head = new_slab;
    alloc->active_list_count += 1;
}

static bool tss_allocator_ready_for_retire(const jfs_tss_allocator_t *alloc) {
    assert(alloc != NULL);
    float percent = alloc->used_obj_count / (float) (alloc->active_list_count * alloc->slab_conf.obj_per_slab); // NOLINT
    return percent <= alloc->percent_to_retire;
}

static void tss_allocator_retire_slabs(jfs_tss_allocator_t *alloc) {
    tss_slab_t **prev_next = &alloc->active_list_head;
    while (*prev_next != NULL) {
        if ((*prev_next)->used_count == 0) {
            tss_slab_t *temp = *prev_next;
            *prev_next = (*prev_next)->next;
            tss_slab_destroy(temp, &alloc->slab_conf);
        } else if ((*prev_next)->used_count <= alloc->slab_conf.retire_threshold) {
            tss_slab_t *temp = *prev_next;
            *prev_next = (*prev_next)->next;
            temp->next = NULL;
            temp->retired_flag = true;
            alloc->active_list_count -= 1;
            // drop pointer on purpose, it will be cleaned up later by free's slow path
        } else {
            prev_next = &(*prev_next)->next;
        }
    }
}
