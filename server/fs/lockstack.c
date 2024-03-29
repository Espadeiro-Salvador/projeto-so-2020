#include "lockstack.h"
#include <errno.h>

/*
 * Initializes the lock stack
 */
void lockstack_init(lockstack_t *stack) {
    stack->first = NULL;
}

/*
 * Returns 1 if the stack contains the lock, otherwise returns 0;
 */
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

/*
 * Adds a lock to the stack
 */
void lockstack_push(lockstack_t *stack, pthread_rwlock_t *lock) {
    if (stack == NULL) {
        return;
    }

    lockstack_node_t *node = malloc(sizeof(lockstack_node_t));
        
    if (node == NULL) {
        fprintf(stderr, "Error: Failed to allocate lock stack node\n");
        exit(EXIT_FAILURE);
    }

    node->lock = lock;
    node->next = stack->first;
    stack->first = node;
}

/*
 * Adds a write lock to the stack if it is not locked already.
 * Returns 1 if the lock is already locked, otherwise returns 0
 */
int lockstack_trylock(lockstack_t *stack, pthread_rwlock_t *lock) {
    if (stack == NULL) return 0;

    int res = pthread_rwlock_trywrlock(lock);
    if (res != EBUSY && res != 0) {
        fprintf(stderr, "Error: Write lock failed to lock\n");
        exit(EXIT_FAILURE);
    } else if (res == EBUSY) {
        return 1;
    } else {
        lockstack_push(stack, lock);
        return 0;
    }
}

/*
 * Adds a read lock to the stack if it does not contain that lock already,
 * locking that lock
 */
void lockstack_addreadlock(lockstack_t *stack, pthread_rwlock_t *lock) {
    if (stack == NULL || lockstack_has(stack, lock)) {
        return;
    }

    if (pthread_rwlock_rdlock(lock)) {
        fprintf(stderr, "Error: Read lock failed to lock\n");
        exit(EXIT_FAILURE);
    }

    lockstack_push(stack, lock);
}

/*
 * Adds a write lock to the stack if it does not contain that lock already,
 * locking that lock
 */
void lockstack_addwritelock(lockstack_t *stack, pthread_rwlock_t *lock) {
    if (stack == NULL || lockstack_has(stack, lock)) {
        return;
    }
    
    if (pthread_rwlock_wrlock(lock)) {
        fprintf(stderr, "Error: Write lock failed to lock\n");
        exit(EXIT_FAILURE);
    }

    lockstack_push(stack, lock);
}

/* 
 * Removes a lock from the stack and unlocks it
 */
void lockstack_pop(lockstack_t *stack) {
    if (stack == NULL) {
        return;
    }

    lockstack_node_t *node = stack->first;
    stack->first = stack->first->next;

    if (pthread_rwlock_unlock(node->lock)) {
        fprintf(stderr, "Error: RWLock failed to unlock\n");
        exit(EXIT_FAILURE);
    }

    free(node);
}

/*
 * Frees all the memory asscociated with the stack
 */
void lockstack_clear(lockstack_t *stack) {
    while (stack->first != NULL) {
        lockstack_pop(stack);
    }
}
