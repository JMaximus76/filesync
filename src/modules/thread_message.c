#include "thread_message.h"
#include "thread_safe_queue.h"
#include <stdlib.h>

struct jfs_tm_queue {
    jfs_tsq_queue_t tsq;
};

jfs_tm_msg_t *jfs_tm_msg_create(jfs_err_t *err) {
    jfs_tm_msg_t *msg = jfs_malloc(sizeof(*msg), err);
    NULL_CHECK_ERR;
    memset(msg, 0, sizeof(*msg));
    return msg;
}

void jfs_tm_msg_destroy(jfs_tm_msg_t *msg_move) {
    if (msg_move == NULL) return;

    if (msg_move->context_free != NULL) {
        msg_move->context_free(msg_move->context);
    } else {
        free(msg_move->context);
    }

    free(msg_move);
}

jfs_tm_queue_t *jfs_tm_queue_create(jfs_err_t *err) {
    jfs_tm_queue_t *queue = jfs_malloc(sizeof(*queue), err);
    NULL_CHECK_ERR;

    jfs_tsq_init(&queue->tsq, (void (*)(void *)) jfs_tm_msg_destroy, err);
    if (*err != JFS_OK) {
        free(queue);
        NULL_RETURN_ERR;
    }

    return queue;
}

void jfs_tm_queue_destroy(jfs_tm_queue_t *queue_move, jfs_err_t *err) {
    if (queue_move == NULL) return;
    jfs_tsq_free(&queue_move->tsq, err);
    VOID_CHECK_ERR;
    free(queue_move);
}

void jfs_tm_queue_push(jfs_tm_queue_t *queue, jfs_tm_msg_t *msg_move, jfs_err_t *err) {
    jfs_tsq_enqueue(&queue->tsq, msg_move, err);
    VOID_CHECK_ERR;
}

jfs_tm_msg_t *jfs_tm_queue_pop(jfs_tm_queue_t *queue, jfs_err_t *err) {
    jfs_tm_msg_t *msg = jfs_tsq_dequeue(&queue->tsq, err);
    NULL_CHECK_ERR;
    return msg;
}

jfs_tm_msg_t *jfs_tm_queue_fast_pop(jfs_tm_queue_t *queue, jfs_err_t *err) {
    jfs_tm_msg_t *msg = jfs_tsq_fast_dequeue(&queue->tsq, err);
    NULL_CHECK_ERR;
    return msg;
}

jfs_tm_msg_t *jfs_tm_queue_timed_pop(jfs_tm_queue_t *queue, const struct timespec *time, jfs_err_t *err) {
    jfs_tm_msg_t *msg = jfs_tsq_timed_dequeue(&queue->tsq, time, err);
    NULL_CHECK_ERR;
    return msg;
}

void jfs_tm_queue_shutdown(jfs_tm_queue_t *queue) {
    jfs_tsq_shutdown(&queue->tsq);
}

jfs_tm_router_t *jfs_tm_router_create(jfs_err_t *err) {
    jfs_tm_router_t *router = jfs_malloc(sizeof(*router), err);
    NULL_CHECK_ERR;

    memset(router, 0, sizeof(*router));
    return router;
}

void jfs_tm_router_destroy(jfs_tm_router_t *router, jfs_err_t *err) {
    for (size_t i = 0; i < JFS_TM_QUEUE_ID_COUNT; i++) {
        VOID_FAIL_IF(!jfs_tsq_is_empty(&router->queues[i]->tsq), JFS_ERR_TM_NOT_SHUTDOWN);
    }

    for (size_t i = 0; i < JFS_TM_QUEUE_ID_COUNT; i++) {
        jfs_tm_queue_destroy(router->queues[i], err);
        if (*err != JFS_OK) {  // should never happen
            RES_ERR;
            pthread_cond_broadcast(&router->queues[i]->tsq.not_empty);
            i -= 1;
        }
    }

    free(router);
}

void jfs_tm_router_add(jfs_tm_router_t *router, jfs_tm_msg_type queue_id, jfs_tm_queue_t *queue_move, jfs_err_t *err) {
    VOID_FAIL_IF(queue_move == NULL || router->queues[queue_id] != NULL, JFS_ERR_ARG);
    router->queues[queue_id] = queue_move;
}

void jfs_tm_router_send(jfs_tm_router_t *router, jfs_tm_msg_type queue_id, jfs_tm_msg_t *msg_move, jfs_err_t *err) {
    jfs_tm_queue_push(router->queues[queue_id], msg_move, err);
    VOID_CHECK_ERR;
}

jfs_tm_msg_t *jfs_tm_router_recv(jfs_tm_router_t *router, jfs_tm_msg_type queue_id, jfs_err_t *err) {
    jfs_tm_msg_t *msg = jfs_tm_queue_pop(router->queues[queue_id], err);
    NULL_CHECK_ERR;
    return msg;
}
