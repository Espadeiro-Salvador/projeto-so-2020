#include "requestqueue.h"
#include "../tecnicofs-api-constants.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct node {
    struct node *next;
    request_t *request;
} node_t;

node_t *head = NULL;
node_t *tail = NULL;

pthread_mutex_t mutex;
pthread_cond_t canRemove;

void init_requestqueue() {
    if (pthread_mutex_init(&mutex, NULL)) {
        fprintf(stderr, "Error: Mutex failed to init");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&canRemove, NULL)) {
        printf("Error: Cond failed to init\n");
        exit(EXIT_FAILURE);
    }
}

void destroy_requestqueue() {
    if (pthread_mutex_destroy(&mutex)) {
        fprintf(stderr, "Error: Mutex failed to destroy\n");
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
void mutex_lock() {
    if (pthread_mutex_lock(&mutex)) {
        printf("Error: Mutex failed to lock\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Unlocks the mutex
 */
void mutex_unlock() {
    if (pthread_mutex_unlock(&mutex)) {
        fprintf(stderr, "Error: Mutex failed to unlock\n");
        exit(EXIT_FAILURE);
    }
}

void add_request(const char *command, struct sockaddr_un client_addr, socklen_t clientlen) {
    mutex_lock();
    node_t *node = malloc(sizeof(node_t));
    if (node == NULL) {
        mutex_unlock();
        return;
    }

    request_t *request = malloc(sizeof(request));
    if (request == NULL) {
        free(node);
        mutex_unlock();
        return;
    }

    request->client_addr = client_addr;
    request->clientlen = clientlen;
    strcpy(request->command, command);
    node->request = request;
    node->next = NULL;

    if (tail == NULL) {
        head = node;
    } else {
        tail->next = node;
    }

    tail = node;

    if (pthread_cond_signal(&canRemove)) {
        fprintf(stderr, "Error: Failed to send cond signal\n");
        exit(EXIT_FAILURE);
    }
    mutex_unlock();
}

request_t *remove_request() {
    mutex_lock();
    while (head == NULL) {
        if (pthread_cond_wait(&canRemove, &mutex)) {
            fprintf(stderr, "Error: couldn't wait for cond signal\n");
            exit(EXIT_FAILURE);
        }
    }
    
    request_t *request = head->request;
    node_t *tmp = head;
    head = head->next;

    if (head == NULL) {
        tail = NULL;
    }

    free(tmp);
    mutex_unlock();

    return request;
}

void destroy_request(request_t *request) {
    free(request);
}
