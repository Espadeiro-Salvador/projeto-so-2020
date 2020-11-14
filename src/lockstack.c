#include "lockstack.h"
#include <errno.h>

void lockstack_init(lockstack_t *stack) {
    stack->first = NULL;
}

int lockstack_has(lockstack_t *stack, pthread_rwlock_t *lock) {
    lockstack_node_t *node = stack->first;

    while (node != NULL) {
        if (node->lock == lock) {
            return 1;
        }
        node = node->next;
    }
    
    return 0;
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

int lockstack_trylock(lockstack_t *stack, pthread_rwlock_t *lock) {
    if (stack == NULL) return 0;

    int res = pthread_rwlock_trywrlock(lock);
    if (res != EBUSY && res != 0) {
        printf("Error: Write lock failed to lock\n");
        exit(EXIT_FAILURE);
    } else if (res == 0) {
        lockstack_push(stack, lock);
    }

    return res == EBUSY;
}

void lockstack_addlock(lockstack_t *stack, pthread_rwlock_t *lock, locktype_t locktype) {
    if (stack == NULL || lockstack_has(stack, lock)) {
        return;
    }

    if (locktype == READ_LOCK) {
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

void lockstack_pop(lockstack_t *stack) {
    if (stack == NULL) {
        return;
    }

    lockstack_node_t *node = stack->first;
    stack->first = stack->first->next;

    if (pthread_rwlock_unlock(node->lock)) {
        printf("Error: RWLock failed to unlock\n");
        exit(EXIT_FAILURE);
    }

    free(node);
}

void lockstack_clear(lockstack_t *stack) {
    while (stack->first != NULL) {
        lockstack_pop(stack);
    }
}
