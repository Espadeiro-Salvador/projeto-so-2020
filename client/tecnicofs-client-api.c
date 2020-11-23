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

int setSocketAddress(char *path, struct sockaddr_un *addr) {
    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

int tfsCreate(char *filename, char nodeType) {
    char msg[64] = "Create Request";
    int res;
    sendto(clientfd, msg, strlen(msg) + 1, 0, (struct sockaddr *)&serv_addr, servlen);
    if (recvfrom(clientfd, &res, sizeof(int), 0,(struct sockaddr *)&serv_addr, &servlen) > 0) {
       if (res == -1) return -1;
        return 0;
    }
    return -1;
}

int tfsDelete(char *path) {
    char msg[64] = "Delete Request";
    int res;
    sendto(clientfd, msg, strlen(msg) + 1, 0, (struct sockaddr *)&serv_addr, servlen);
    if (recvfrom(clientfd, &res, sizeof(int), 0,(struct sockaddr *)&serv_addr, &servlen) > 0) {
        if (res == -1) return -1;
        return 0;
    }
    return -1;
}

int tfsMove(char *from, char *to) {
    char msg[64] = "Move Request";
    int res;
    sendto(clientfd, msg, strlen(msg) + 1, 0, (struct sockaddr *)&serv_addr, servlen);
    if (recvfrom(clientfd, &res, sizeof(int), 0,(struct sockaddr *)&serv_addr, &servlen) > 0) {
        if (res == -1) return -1;
        return 0;
    }
    return -1;
}

int tfsLookup(char *path) {
    char msg[64] = "Search Request";
    int res;
    sendto(clientfd, msg, strlen(msg) + 1, 0, (struct sockaddr *)&serv_addr, servlen);
    if (recvfrom(clientfd, &res, sizeof(int), 0,(struct sockaddr *)&serv_addr, &servlen) > 0) {
        if (res == -1) return -1;
        return 0;
    }
    return -1;
}

int tfsMount(char *sockPath) {
    clientfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (clientfd < 0)
        return -1;

    unlink("/tmp/client");
    clientlen = setSocketAddress("/tmp/client", &client_addr);
    if(bind(clientfd, (struct sockaddr *) &client_addr, clientlen)) {
      return -1;
    }
    servlen = setSocketAddress(sockPath, &serv_addr);
    return 0;
}

int tfsUnmount() {
    return -1;
}
