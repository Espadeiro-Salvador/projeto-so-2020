#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <strings.h>

#include "fs/operations.h"
#include "../tecnicofs-api-constants.h"

int serverfd;

int setSocketAddress(char *path, struct sockaddr_un *addr) {
    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

int processCommand(const char *command) {
    int response = FAIL;

    char token;
    char arg1[MAX_INPUT_SIZE];
    char arg2[MAX_INPUT_SIZE];
    int numTokens = sscanf(command, "%c %s %s", &token, arg1, arg2);

    if (numTokens < 2) {
        return response;
    }

    switch (token) {
        case 'c':
            if (numTokens != 3)
                return response;
            switch (arg2[0]) {
                case 'f':
                    response = create(arg1, T_FILE);
                    break;
                case 'd':
                    response = create(arg1, T_DIRECTORY);
                    break;
            }
            break;
        case 'l': 
            if (numTokens != 2)
                return response;
            response = lookup(arg1);
            break;

        case 'd':
            if (numTokens != 2) 
                return response;
            response = delete(arg1);
            break;

        case 'm':
            if (numTokens != 3)
                return response;
            response = move(arg1, arg2);
            break;
        case 'p':
            if (numTokens != 2) 
                return response;
            response = print_tree(arg1);
            break;
    }
    return response;
}

int receiveCommand(char *command, struct sockaddr_un *client_addr, socklen_t *clientlen) {
    int msglen = recvfrom(serverfd, command, sizeof(char) * (MAX_INPUT_SIZE - 1), 0,
                          (struct sockaddr *)client_addr, clientlen);
    if (msglen > 0) {
        command[msglen] = '\0';
    }

    return msglen <= 0;
}

int sendResponse(int response, struct sockaddr_un *client_addr, socklen_t clientlen) {
    return sendto(serverfd, &response, sizeof(int), 0, (struct sockaddr *) client_addr, clientlen) <= 0;
}

void *threadFunction() {
    while (1) {
        struct sockaddr_un client_addr;
        socklen_t clientlen = sizeof(struct sockaddr_un);

        char command[MAX_INPUT_SIZE];
        if (receiveCommand(command, &client_addr, &clientlen))
            continue;

        int response = processCommand(command);
        sendResponse(response, &client_addr, clientlen);
    }

    return NULL;
}

/*
 * Creates the number of threads given
 */
void create_thread_pool(pthread_t *tid, int numberThreads) {
    for(int i = 0; i < numberThreads; i++) {
        if (pthread_create(&tid[i], NULL, threadFunction, NULL) != 0) {
            printf("Error: could not create thread\n");
            exit(EXIT_FAILURE);
        }
    }
}

/*
 * Wait for the all the threads
 */
void wait_for_threads(pthread_t *tid, int numberThreads) {
    for (int i = 0; i < numberThreads; i++) {
        if (pthread_join(tid[i], NULL)) {
            printf("Error: error waiting for thread\n");
            exit(EXIT_FAILURE);
        }
    }
}

void init_server(char *path) {
    struct sockaddr_un server_addr;
    socklen_t addrlen;

    serverfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (serverfd < 0) {
        fprintf(stderr, "Error: server cannot open socket\n");
        exit(EXIT_FAILURE);
    }

    unlink(path);

    addrlen = setSocketAddress(path, &server_addr);
    if (bind(serverfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        fprintf(stderr, "Error: server could not bind socket\n");
        exit(EXIT_FAILURE);
    }

    if (chmod(path, 00222) == -1) {
        fprintf(stderr, "Error: can't change permissions of socket\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Validates the number of threads and the number of args 
 */
int parse_args(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Error: wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    /* get number of threads */
    int numberThreads = atoi(argv[1]);
    if (numberThreads < 1) {
        printf("Error: can't run less than one thread\n");
        exit(EXIT_FAILURE);
    }

    return numberThreads;
}

int main(int argc, char* argv[]) {
    int numberThreads = parse_args(argc, argv);

    init_server(argv[2]);
    init_fs(); 

    /* create the thread pool */
    pthread_t tid[numberThreads];
    create_thread_pool(tid, numberThreads);
    wait_for_threads(tid, numberThreads);

    destroy_fs();
    unlink(argv[2]);
    
    exit(EXIT_SUCCESS);
}
