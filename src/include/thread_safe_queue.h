#ifndef JFS_THREAD_SAFE_QUEUE_H
#define JFS_THREAD_SAFE_QUEUE_H

#include "error.h"
#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>

#define JFS_TSQ_CHUNK_SIZE 64

typedef struct jfs_tsq_queue jfs_tsq_queue_t;
typedef struct jfs_tsq_chunk jfs_tsq_chunk_t;

struct jfs_tsq_chunk {
    void            *items[JFS_TSQ_CHUNK_SIZE];
    size_t           count;
    size_t           head;
    size_t           tail;
    jfs_tsq_chunk_t *next;
};

struct jfs_tsq_queue {
    pthread_mutex_t  lock;
    int              eventfd;
    jfs_tsq_chunk_t *front;
    jfs_tsq_chunk_t *back;
    size_t           chunk_count;
    void (*free_item)(void *);
};

void  jfs_tsq_init(jfs_tsq_queue_t *queue_init, void (*free_item)(void *), jfs_err_t *err);
void  jfs_tsq_free(jfs_tsq_queue_t *queue_free, jfs_err_t *err);
void  jfs_tsq_enqueue(jfs_tsq_queue_t *queue, void *item, jfs_err_t *err);
void *jfs_tsq_dequeue(jfs_tsq_queue_t *queue, jfs_err_t *err) WUR;
bool  jfs_tsq_is_free_ready(jfs_tsq_queue_t *queue) WUR;

jfs_tsq_chunk_t *jfs_tsq_chunk_create(jfs_err_t *err) WUR;
void             jfs_tsq_chunk_destroy(jfs_tsq_chunk_t *chunk, void (*free_item)(void *));
void             jfs_tsq_chunk_enqueue(jfs_tsq_chunk_t *chunk, void *item);
void            *jfs_tsq_chunk_dequeue(jfs_tsq_chunk_t *chunk) WUR;

#endif
