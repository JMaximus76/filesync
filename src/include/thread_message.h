#ifndef JFS_THREAD_MESSAGE_H
#define JFS_THREAD_MESSAGE_H

#include "error.h"
#include <pthread.h>
#include <stdatomic.h>
#include <sys/types.h>

typedef struct jfs_tm_msg    jfs_tm_msg_t;
typedef struct jfs_tm_queue  jfs_tm_queue_t;
typedef struct jfs_tm_router jfs_tm_router_t;

typedef union {
} jfs_tm_kind_t;

typedef enum {
    JFS_TM_NORM = 0,
    JFS_TM_RET,
} jfs_tm_msg_type_t;

typedef enum {
    JFS_TM_DATABASE = 0,
    JFS_TM_GENERAL,
    JFS_TM_MAIN,
    JFS_TM_NETWORK,
    JFS_TM_QUEUE_ID_COUNT
} jfs_tm_location_t;

struct jfs_tm_msg_route {
    jfs_tm_location_t sender;
    jfs_tm_location_t target;
    jfs_tm_location_t forward;
};

struct jfs_tm_msg {
    jfs_tm_msg_type_t type;
    jfs_tm_location_t sender;
    jfs_tm_location_t target;
    jfs_tm_kind_t     kind;
    void             *context;
    void (*context_free)(void *);
};

struct jfs_tm_router {
    jfs_tm_queue_t *queues[JFS_TM_QUEUE_ID_COUNT];
};

jfs_tm_msg_t *jfs_tm_msg_create(jfs_err_t *err);
void          jfs_tm_msg_destroy(jfs_tm_msg_t *msg_move);

jfs_tm_queue_t *jfs_tm_queue_create(jfs_err_t *err);
void            jfs_tm_queue_destroy(jfs_tm_queue_t *queue_move, jfs_err_t *err);
void            jfs_tm_queue_push(jfs_tm_queue_t *queue, jfs_tm_msg_t *msg_move, jfs_err_t *err);
jfs_tm_msg_t   *jfs_tm_queue_pop(jfs_tm_queue_t *queue, jfs_err_t *err);
jfs_tm_msg_t   *jfs_tm_queue_fast_pop(jfs_tm_queue_t *queue, jfs_err_t *err);
jfs_tm_msg_t   *jfs_tm_queue_timed_pop(jfs_tm_queue_t *queue, const struct timespec *time, jfs_err_t *err);
void            jfs_tm_queue_shutdown(jfs_tm_queue_t *queue);

jfs_tm_router_t *jfs_tm_router_create(jfs_err_t *err);
void             jfs_tm_router_destroy(jfs_tm_router_t *router, jfs_err_t *err);
void             jfs_tm_router_add(jfs_tm_router_t *router, jfs_tm_location_t queue_id, jfs_tm_queue_t *queue_move, jfs_err_t *err);
void             jfs_tm_router_send(jfs_tm_router_t *router, jfs_tm_location_t queue_id, jfs_tm_msg_t *msg_move, jfs_err_t *err);
jfs_tm_msg_t    *jfs_tm_router_recv(jfs_tm_router_t *router, jfs_tm_location_t queue_id, jfs_err_t *err);

#endif
