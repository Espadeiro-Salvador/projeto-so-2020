#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

int clientfd;
socklen_t servlen, clientlen;
struct sockaddr_un serv_addr, client_addr;
char clientPath[22];

int setSocketAddress(char *path, struct sockaddr_un *addr) {
    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

int sendCommand(const char *command) {
    return sendto(clientfd, command, strlen(command) + 1, 0, (struct sockaddr *)&serv_addr, servlen) <= 0;
}

int receiveResponse() {
    int response;
    if (recvfrom(clientfd, &response, sizeof(int), 0, NULL, NULL) > 0) {
        return response;
    }

    return -1;
}

int tfsCreate(char *filename, char nodeType) {
    char command[MAX_INPUT_SIZE];
    if (sprintf(command, "c %s %c", filename, nodeType) < 0)
        return -1;

    if (sendCommand(command)) {
        return -1;
    }

    return receiveResponse();
}

int tfsDelete(char *path) {
    char command[MAX_INPUT_SIZE];
    if (sprintf(command, "d %s", path) < 0)
        return -1;
    
    if (sendCommand(command)) {
        return -1;
    }

    return receiveResponse();
}

int tfsMove(char *from, char *to) {
    char command[MAX_INPUT_SIZE];
    if (sprintf(command, "m %s %s", from, to) < 0)
        return -1;
    
    if (sendCommand(command)) {
        return -1;
    }

    return receiveResponse();
}

int tfsLookup(char *path) {
    char command[MAX_INPUT_SIZE];
    if (sprintf(command, "l %s", path) < 0)
        return -1;

    if(sendCommand(command)) {
        return -1;
    }

    return receiveResponse();
}

int tfsMount(char *sockPath) {
    clientfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (clientfd < 0)
        return -1;
    
    if (sprintf(clientPath, "/tmp/tfs-client-%d", getpid()) < 0)
        return -1;

    unlink(clientPath);
    clientlen = setSocketAddress(clientPath, &client_addr);
    if(bind(clientfd, (struct sockaddr *) &client_addr, clientlen))
      return -1;
    servlen = setSocketAddress(sockPath, &serv_addr);

    return 0;
}

int tfsUnmount() {
    unlink(clientPath);
    return -1;
}
