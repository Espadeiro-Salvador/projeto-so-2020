#ifndef REQUESTQUEUE_H
#define REQUESTQUEUE_H

#include <sys/socket.h>
#include <sys/un.h>
#include "../tecnicofs-api-constants.h"

typedef struct request {
    char command[MAX_INPUT_SIZE];
    struct sockaddr_un client_addr;
    socklen_t clientlen;
} request_t;


void init_requestqueue();
void destroy_requestqueue();
void add_request(const char *command, struct sockaddr_un client_addr, socklen_t clientlen);
request_t *remove_request();
void destroy_request(request_t *request);

#endif /* REQUESTQUEUE_H */
