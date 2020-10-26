#ifndef LOCKSTACK_H
#define LOCKSTACK_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "tecnicofs-api-constants.h"

typedef struct lockstack_node_t {
    pthread_rwlock_t *lock;
    struct lockstack_node_t *next;
} lockstack_node_t;

typedef struct lockstack_t {
    lockstack_node_t *first;
} lockstack_t;

typedef enum locktype_t {
    READ_LOCK, WRITE_LOCK, NO_LOCK
} locktype_t;

void lockstack_init(lockstack_t *stack);
void lockstack_push(lockstack_t *stack, pthread_rwlock_t *lock, locktype_t locktype);
void lockstack_clear(lockstack_t *stack);

#endif /* LOCKSTACK_H */
