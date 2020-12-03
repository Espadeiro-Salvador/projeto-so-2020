#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>

int clientfd;
socklen_t servlen, clientlen;
struct sockaddr_un serv_addr, client_addr;
char clientPath[22];

/*
 * Initializes the unix socket address.
 * Input:
 * - path: path to socket
 * - addr: pointer to address
 */
int setSocketAddress(char *path, struct sockaddr_un *addr) {
    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

/*
 * Sends command to the server socket.
 */
int sendCommand(const char *command) {
    return sendto(clientfd, command, strlen(command) + 1, 0, (struct sockaddr *)&serv_addr, servlen) <= 0;
}

/*
 * Receives response from the server socket.
 */
int receiveResponse() {
    int response;
    if (recvfrom(clientfd, &response, sizeof(int), 0, NULL, NULL) > 0)
        return response;

    return -1;
}

/*
 * Sends create command to the server socket.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: response from the server socket.
 */
int tfsCreate(char *filename, char nodeType) {
    char command[MAX_INPUT_SIZE];
    if (sprintf(command, "c %s %c", filename, nodeType) < 0)
        return -1;

    if (sendCommand(command))
        return -1;

    return receiveResponse();
}

/*
 * Sends delete command to the server socket.
 * Input:
 *  - name: path of node
 * Returns: response from the server socket.
 */
int tfsDelete(char *path) {
    char command[MAX_INPUT_SIZE];
    if (sprintf(command, "d %s", path) < 0)
        return -1;
    
    if (sendCommand(command))
        return -1;

    return receiveResponse();
}

/*
 * Sends move command to the server socket.
 * Input:
 *  - from: path of node to move
 *  - to: destination path of node
 * Returns: response from the server socket.
 */
int tfsMove(char *from, char *to) {
    char command[MAX_INPUT_SIZE];
    if (sprintf(command, "m %s %s", from, to) < 0)
        return -1;
    
    if (sendCommand(command))
        return -1;

    return receiveResponse();
}

/*
 * Sends lookup command to the server socket.
 * Input:
 *  - name: path of node
 * Returns: response from the server socket.
 */
int tfsLookup(char *path) {
    char command[MAX_INPUT_SIZE];
    if (sprintf(command, "l %s", path) < 0)
        return -1;

    if(sendCommand(command))
        return -1;

    return receiveResponse();
}

/*
 * Sends print command to the server socket.
 * Input:
 *  - outputfile: path for the file to output the tree
 * Returns: response from the server socket.
 */
int tfsPrint(char *outputfile) {
    char command[MAX_INPUT_SIZE];
    if (sprintf(command, "p %s", outputfile) < 0)
        return -1;

    if(sendCommand(command))
        return -1;

    return receiveResponse();
}

/*
 * Creates client socket and sets the server address from the path.
 * Input:
 *  - sockPath: path of the server socket
 * Returns: 0 if successful, -1 if failed
 */
int tfsMount(char *sockPath) {
    clientfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (clientfd < 0)
        return -1;
    
    if (sprintf(clientPath, "/tmp/tfs-client-%d", getpid()) < 0)
        return -1;

    if (unlink(clientPath) && errno != ENOENT)
        return -1;
    
    clientlen = setSocketAddress(clientPath, &client_addr);
    if(bind(clientfd, (struct sockaddr *) &client_addr, clientlen))
      return -1;
    servlen = setSocketAddress(sockPath, &serv_addr);

    return 0;
}

/*
 * Unlinks the client socket.
 * Returns: 0 if successful, -1 if failed
 */
int tfsUnmount() {
    return unlink(clientPath);
}
