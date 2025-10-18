#include "lru_cache.h"
#include <assert.h>

static void lru_promote(jfs_lru_t *lru, size_t index);

void jfs_lru_init(jfs_lru_t *lru_init, const jfs_lru_conf_t *conf, jfs_err_t *err) {
    VOID_FAIL_IF(conf->cmp == NULL, JFS_ERR_BAD_CONF);
    VOID_FAIL_IF(conf->hit == NULL, JFS_ERR_BAD_CONF);
    VOID_FAIL_IF(conf->miss == NULL, JFS_ERR_BAD_CONF);
    VOID_FAIL_IF(conf->evict == NULL, JFS_ERR_BAD_CONF);

    jfs_mb_init(&lru_init->mb, &conf->mb_conf, err);
    VOID_CHECK_ERR;

    lru_init->count = 0;
    lru_init->cmp = conf->cmp;
    lru_init->hit = conf->hit;
    lru_init->miss = conf->miss;
    lru_init->evict = conf->evict;
    lru_init->evict_ctx = conf->evict_ctx;
}

void jfs_lru_free(jfs_lru_t *lru_move) {
    for (size_t i = 0; i < lru_move->count; i++) {
        lru_move->evict(jfs_mb_index(&lru_move->mb, i), lru_move->evict_ctx);
    }
}

void jfs_lru_access(jfs_lru_t *lru, const void *key, void *user_ctx) { // NOLINT
    for (size_t i = 0; i < lru->count; i++) {
        void *slot_ptr = jfs_mb_index(&lru->mb, i);
        if (lru->cmp(key, slot_ptr) == 0) {
            lru->hit(slot_ptr, user_ctx);
            if (i != 0) lru_promote(lru, i);
            return;
        }
    }

    void *slot = NULL;
    if (lru->count < lru->mb.capacity) {
        slot = jfs_mb_index(&lru->mb, lru->count);
        lru->count += 1;
    } else {
        assert(lru->count == lru->mb.capacity);
        slot = jfs_mb_index(&lru->mb, lru->count - 1);
        lru->evict(slot, lru->evict_ctx);
    }

    lru->miss(slot, user_ctx);
    lru_promote(lru, lru->count - 1);
}

static void lru_promote(jfs_lru_t *lru, size_t index) {
    uint8_t temp_slot[lru->mb.obj_size];
    memcpy(temp_slot, jfs_mb_index(&lru->mb, index), lru->mb.obj_size);
    memmove(jfs_mb_index(&lru->mb, 1), lru->mb.base_ptr, index * lru->mb.obj_size);
    memcpy(lru->mb.base_ptr, temp_slot, lru->mb.obj_size);
}
