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
char clientPath[MAX_CLIENT_PATH];

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
 * Sends command to the server socket and returns 0 if it is successful.
 */
int sendCommand(const char *command) {
    return sendto(clientfd, command, strlen(command) + 1, 0, (struct sockaddr *)&serv_addr, servlen) <= 0;
}

/*
 * Receives response from the server socket, returns it if successful, if not it returns FAIL.
 */
int receiveResponse() {
    int response;
    if (recvfrom(clientfd, &response, sizeof(int), 0, NULL, NULL) > 0)
        return response;

    return FAIL;
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
        return FAIL;

    if (sendCommand(command))
        return FAIL;

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
        return FAIL;
    
    if (sendCommand(command))
        return FAIL;

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
        return FAIL;
    
    if (sendCommand(command))
        return FAIL;

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
        return FAIL;

    if(sendCommand(command))
        return FAIL;

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
        return FAIL;

    if(sendCommand(command))
        return FAIL;

    return receiveResponse();
}

/*
 * Creates client socket and sets the server address from the path.
 * Input:
 *  - sockPath: path of the server socket
 * Returns: SUCCESS or FAIL
 */
int tfsMount(char *sockPath) {
    clientfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (clientfd < 0)
        return FAIL;
    
    if (sprintf(clientPath, "/tmp/tfs-client-%d", getpid()) < 0)
        return FAIL;

    if (unlink(clientPath) && errno != ENOENT)
        return FAIL;
    
    clientlen = setSocketAddress(clientPath, &client_addr);
    if(bind(clientfd, (struct sockaddr *) &client_addr, clientlen))
      return FAIL;
    servlen = setSocketAddress(sockPath, &serv_addr);

    return SUCCESS;
}

/*
 * Unlinks the client socket.
 * Returns: SUCCESS or FAIL
 */
int tfsUnmount() {
    return unlink(clientPath);
}
