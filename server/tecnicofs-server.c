#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/socket.h>
#include <strings.h>

#include "../tecnicofs-api-constants.h"

int setSocketAddress(char *path, struct sockaddr_un *addr) {
    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

void *threadFunction() {
    return NULL;
}

/*
 * Creates the number of threads given
*/
//void create_thread_pool(pthread_t *tid, int numberThreads) {
//    for(int i = 0; i < numberThreads; i++) {
//        if (pthread_create(&tid[i], NULL, threadFunction, NULL) != 0) {
//            printf("Error: could not create thread\n");
//            exit(EXIT_FAILURE);
//        }
//    }
//}

int main(int argc, char* argv[]) {
    int serverfd;
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

    /* create the tasks */
    //int numberThreads = atoi(argv[1]);
    //pthread_t tid[numberThreads];
    //create_thread_pool(tid, atoi(argv[1]));

    while (1) {
        struct sockaddr_un client_addr;
        char command[MAX_INPUT_SIZE];
        int msglen;
        int output;

        addrlen = sizeof(struct sockaddr_un);
        msglen = recvfrom(serverfd, command, sizeof(command) - 1, 0,
                          (struct sockaddr *)&client_addr, &addrlen);
        if (msglen <= 0) 
            continue;
        command[msglen] = '\0';
        printf("%s\n", command);

        output = -1;

        sendto(serverfd, &output, sizeof(int), 0, (struct sockaddr *)&client_addr, addrlen);
    }
    
    exit(EXIT_SUCCESS);
}
