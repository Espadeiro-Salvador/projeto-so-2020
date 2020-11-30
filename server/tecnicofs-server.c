#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/socket.h>
#include <strings.h>

#include "requestqueue.h"
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
            switch (arg2[0]) {
                case 'f':
                    printf("Create file: %s\n", arg1);
                    response = create(arg1, T_FILE);
                    break;
                case 'd':
                    printf("Create directory: %s\n", arg1);
                    response = create(arg1, T_DIRECTORY);
                    break;
            }
            break;
        case 'l': 
            response = lookup(arg1);

            if (response >= 0)
                printf("Search: %s found\n", arg1);
            else
                printf("Search: %s not found\n", arg1);
            break;

        case 'd':
            printf("Delete: %s\n", arg1);
            response = delete(arg1);
            break;

        case 'm':
            printf("Move: %s to %s\n", arg1, arg2);
            response = move(arg1, arg2);
            break;
    }
    return response;
}

void *threadFunction() {
    while (1) {
        request_t *request = remove_request();

        int response = processCommand(request->command);
        sendto(serverfd, &response, sizeof(int), 0, (struct sockaddr *) &request->client_addr, request->clientlen);
        free(request);
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
void sendStatus(int serverfd, int *retvalue, struct sockaddr *client_addr, socklen_t addrlen) {
    if (sendto(serverfd, &retvalue, sizeof(int), 0, (struct sockaddr *)client_addr, addrlen) < 0) {
        
    }
}
*/
int main(int argc, char* argv[]) {
    struct sockaddr_un server_addr;
    socklen_t addrlen;

    serverfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (serverfd < 0) {
        fprintf(stderr, "Error: server cannot open socket\n");
        exit(EXIT_FAILURE);
    }

    unlink(argv[2]);

    addrlen = setSocketAddress(argv[2], &server_addr);
    if (bind(serverfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        fprintf(stderr, "Error: server could not bind socket\n");
        exit(EXIT_FAILURE);
    }

    /* PERMISSOES ??? */
    init_fs();

    /* create the tasks */
    int numberThreads = atoi(argv[1]);
    pthread_t tid[numberThreads];
    create_thread_pool(tid, atoi(argv[1]));

    while (1) {
        struct sockaddr_un client_addr;
        char command[MAX_INPUT_SIZE];
        int msglen;

        addrlen = sizeof(struct sockaddr_un);
        msglen = recvfrom(serverfd, command, sizeof(command) - 1, 0,
                          (struct sockaddr *)&client_addr, &addrlen);
                          
        if (msglen <= 0) 
            continue;
        command[msglen] = '\0';
        printf("%s\n", command);
        add_request(command, client_addr, addrlen);
    }

    destroy_fs();
    
    exit(EXIT_SUCCESS);
}
