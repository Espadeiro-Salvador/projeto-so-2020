#ifndef API_H
#define API_H

#include "../tecnicofs-api-constants.h"

#define SUCCESS 0
#define FAIL -1
#define MAX_CLIENT_PATH 22

int tfsCreate(char *path, char nodeType);
int tfsDelete(char *path);
int tfsLookup(char *path);
int tfsMove(char *from, char *to);
int tfsPrint(char *outputfile);
int tfsMount(char *serverName);
int tfsUnmount();

#endif /* CLIENT_H */
