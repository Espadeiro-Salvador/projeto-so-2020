#include "lockstack.h"

void lockstack_init(lockstack_t *stack) {
    stack->first = NULL;
}

void lockstack_addlock(lockstack_t *stack, pthread_rwlock_t *lock, locktype_t locktype) {
    if (locktype == NO_LOCK || stack == NULL) {
        return;
    } else if (locktype == READ_LOCK) {
        if (pthread_rwlock_rdlock(lock)) {
            printf("Error: Read lock failed to lock\n");
            exit(EXIT_FAILURE);
        }
    } else if (locktype == WRITE_LOCK) {
        if (pthread_rwlock_wrlock(lock)) {
            printf("Error: Write lock failed to lock\n");
            exit(EXIT_FAILURE);
        }
    }

    lockstack_push(stack, lock);
}

void lockstack_push(lockstack_t *stack, pthread_rwlock_t *lock) {
    if (stack == NULL) {
        return;
    }

    lockstack_node_t *node = malloc(sizeof(lockstack_node_t));
        
    if (node == NULL) {
        printf("Error: Failed to allocate lock stack node\n");
        exit(EXIT_FAILURE);
    }

    node->lock = lock;
    node->next = stack->first;
    stack->first = node;
}

void lockstack_clear(lockstack_t *stack) {
    lockstack_node_t *node = stack->first;
    lockstack_node_t *temp;

    while (node != NULL) {
        if (pthread_rwlock_unlock(node->lock)) {
            printf("Error: RWLock failed to unlock\n");
            exit(EXIT_FAILURE);
        }

        temp = node;
        node = node->next;
        free(temp);
    }
}
