#include "printsync.h"

pthread_mutex_t mutex;
pthread_cond_t canModify, canPrint;

/*
 * Initializes the mutex and the conds
 */
void init_printsync() {
    if (pthread_mutex_init(&mutex, NULL)) {
        printf("Error: Mutex failed to init\n");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_cond_init(&canModify, NULL)) {
        printf("Error: Cond failed to init\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&canPrint, NULL)) {
        printf("Error: Cond failed to init\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Destroys the mutex and the conds
 */
void destroy_printsync() {
    if (pthread_mutex_destroy(&mutex)) {
        printf("Error: Mutex failed to destroy\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_destroy(&canModify)) {
        printf("Error: Cond failed to destroy\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_destroy(&canPrint)) {
        printf("Error: Cond failed to destroy\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Locks the mutex
 */
void printsync_lock() {
    if (pthread_mutex_lock(&mutex)) {
        printf("Error: Mutex failed to lock\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Unlocks the mutex
 */
void printsync_unlock() {
    if (pthread_mutex_unlock(&mutex)) {
        printf("Error: Mutex failed to unlock\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Waits for the signal from the print task
 */
void wait_for_print_task() {
    if (pthread_cond_wait(&canModify, &mutex)) {
        printf("Error: couldn't wait for cond signal\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Waits for the signal from the modifying tasks
 */
void wait_for_modifying_tasks() {
    if (pthread_cond_wait(&canPrint, &mutex)) {
        printf("Error: couldn't wait for cond signal\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Sends a signal to the modifying tasks
 */
void broadcast_modifying_tasks() {
    if (pthread_cond_broadcast(&canModify)) {
        printf("Error: Failed to send cond signal\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Sends a signal to the print task
 */
void signal_print_task() {
    if (pthread_cond_signal(&canPrint)) {
        printf("Error: Failed to send cond signal\n");
        exit(EXIT_FAILURE);
    }
}
