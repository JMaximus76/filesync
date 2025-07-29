#include "thread_safe_queue.h"
#include "error.h"
#include "file_io.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <unistd.h>

void jfs_tsq_init(jfs_tsq_queue_t *queue_init, void (*free_item)(void *), jfs_err_t *err) {
    VOID_FAIL_IF(free_item == NULL, JFS_ERR_ARG);

    jfs_tsq_chunk_t *first_chunk = jfs_tsq_chunk_create(err);
    VOID_CHECK_ERR;

    int event_fd = jfs_eventfd(0, EFD_SEMAPHORE, err);
    if (err != JFS_OK) {
        jfs_tsq_chunk_destroy(first_chunk, free_item);
        VOID_RETURN_ERR;
    }

    jfs_mutex_init(&queue_init->lock, NULL, err);
    if (err != JFS_OK) {
        jfs_tsq_chunk_destroy(first_chunk, free_item);
        close(event_fd); // don't care if there is an error on close
        VOID_RETURN_ERR;
    }

    queue_init->back = first_chunk;
    queue_init->front = first_chunk;
    queue_init->chunk_count = 1;
    queue_init->free_item = free_item;
}

void jfs_tsq_free(jfs_tsq_queue_t *queue_free, jfs_err_t *err) {
    pthread_mutex_lock(&queue_free->lock);
    if (*err != JFS_OK) {
        pthread_mutex_unlock(&queue_free->lock);
        VOID_RETURN_ERR;
    }

    while (queue_free->front != NULL) {
        jfs_tsq_chunk_t *temp = queue_free->front->next;
        jfs_tsq_chunk_destroy(queue_free->front, queue_free->free_item);
        queue_free->front = temp;
    }

    pthread_mutex_unlock(&queue_free->lock);

    jfs_mutex_destroy(&queue_free->lock, err);
    VOID_CHECK_ERR;
}

void jfs_tsq_enqueue(jfs_tsq_queue_t *queue, void *item, jfs_err_t *err) {
    pthread_mutex_lock(&queue->lock);

    if (queue->back->count == JFS_TSQ_CHUNK_SIZE) {
        jfs_tsq_chunk_t *new_chunk = jfs_tsq_chunk_create(err);
        if (*err != JFS_OK) {
            pthread_mutex_unlock(&queue->lock);
            VOID_RETURN_ERR;
        }
        queue->back->next = new_chunk;
        queue->back = new_chunk;
        queue->chunk_count += 1;
    }

    jfs_fio_write(queue->eventfd, &(uint64_t) {1}, sizeof(uint64_t), err);
    VOID_CHECK_ERR;
    jfs_tsq_chunk_enqueue(queue->back, item);
    pthread_mutex_unlock(&queue->lock);
}

void *jfs_tsq_dequeue(jfs_tsq_queue_t *queue, jfs_err_t *err) {
    pthread_mutex_lock(&queue->lock);
    if (queue->front->count == 0) {
        pthread_mutex_unlock(&queue->lock);
        *err = JFS_ERR_EMPTY;
        NULL_RETURN_ERR;
    }

    void *item = jfs_tsq_chunk_dequeue(queue->front);

    if (queue->chunk_count > 1 && queue->front->count == 0) {
        jfs_tsq_chunk_t *old_chunk = queue->front;
        queue->front = queue->front->next;
        jfs_tsq_chunk_destroy(old_chunk, queue->free_item);
        queue->chunk_count -= 1;
    }

    pthread_mutex_unlock(&queue->lock);
    return item;
}

bool jfs_tsq_is_free_ready(jfs_tsq_queue_t *queue) {
    pthread_mutex_lock(&queue->lock);
    bool is_free_ready = queue->front->count == 0 && queue->chunk_count == 1;
    pthread_mutex_unlock(&queue->lock);
    return is_free_ready;
}

jfs_tsq_chunk_t *jfs_tsq_chunk_create(jfs_err_t *err) {
    jfs_tsq_chunk_t *chunk = jfs_malloc(sizeof(*chunk), err);
    NULL_CHECK_ERR;
    memset(chunk, 0, sizeof(*chunk));
    return chunk;
}

void jfs_tsq_chunk_destroy(jfs_tsq_chunk_t *chunk, void (*free_item)(void *)) {
    while (chunk->count != 0) {
        free_item(chunk->items[chunk->head]);
        chunk->head = (chunk->head + 1) % JFS_TSQ_CHUNK_SIZE;
    }
    free(chunk);
}

void jfs_tsq_chunk_enqueue(jfs_tsq_chunk_t *chunk, void *item) {
    chunk->items[chunk->tail] = item;
    chunk->tail = (chunk->tail + 1) % JFS_TSQ_CHUNK_SIZE;
    chunk->count += 1;
}

void *jfs_tsq_chunk_dequeue(jfs_tsq_chunk_t *chunk) {
    void *item = chunk->items[chunk->head];
    chunk->head = (chunk->head + 1) % JFS_TSQ_CHUNK_SIZE;
    chunk->count -= 1;
    return item;
}
