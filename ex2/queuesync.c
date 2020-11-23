#include "queuesync.h"

pthread_mutex_t mutex;
pthread_cond_t canInsert, canRemove;

/*
 * Initializes the mutex and the conds
 */
void init_sync() {
    if (pthread_mutex_init(&mutex, NULL)) {
        printf("Error: Mutex failed to init\n");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_cond_init(&canInsert, NULL)) {
        printf("Error: Cond failed to init\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&canRemove, NULL)) {
        printf("Error: Cond failed to init\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Destroys the mutex and the conds
 */
void destroy_sync() {
    if (pthread_mutex_destroy(&mutex)) {
        printf("Error: Mutex failed to destroy\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_destroy(&canInsert)) {
        printf("Error: Cond failed to destroy\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_destroy(&canRemove)) {
        printf("Error: Cond failed to destroy\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Locks the mutex
 */
void lock_command_queue() {
    if (pthread_mutex_lock(&mutex)) {
        printf("Error: Mutex failed to lock\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Unlocks the mutex
 */
void unlock_command_queue() {
    if (pthread_mutex_unlock(&mutex)) {
        printf("Error: Mutex failed to unlock\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Waits for the signal from the consumer
 */
void wait_for_consumer() {
    if (pthread_cond_wait(&canInsert, &mutex)) {
        printf("Error: couldn't wait for cond signal\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Waits for the signal from the producer
 */
void wait_for_producer() {
    if (pthread_cond_wait(&canRemove, &mutex)) {
        printf("Error: couldn't wait for cond signal\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Sends a signal to the producer
 */
void signal_producer() {
    if (pthread_cond_signal(&canInsert)) {
        printf("Error: Failed to send cond signal\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Sends a signal to the consumer
 */
void signal_consumer() {
    if (pthread_cond_signal(&canRemove)) {
        printf("Error: Failed to send cond signal\n");
        exit(EXIT_FAILURE);
    }
}
