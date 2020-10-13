#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>
#include "fs/operations.h" 

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

enum { MUTEX, RWLOCK, NOSYNC } syncStrategy;
pthread_mutex_t mutex;
pthread_rwlock_t rwlock;

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

void checkError(int error, const char *msg) {
    if (error) {
        printf("Error: %s.\n", msg);
        exit(EXIT_FAILURE);
    }
}

void lockWriteFS() {
    if (syncStrategy == MUTEX) {
        checkError(pthread_mutex_lock(&mutex), "Mutex failed to lock");
    } else if (syncStrategy == RWLOCK) {
        pthread_rwlock_wrlock(&rwlock);
    }
}

void lockReadFS() {
    if (syncStrategy == MUTEX) {
        pthread_mutex_lock(&mutex);
    } else if (syncStrategy == RWLOCK) {
        pthread_rwlock_rdlock(&rwlock);
    }
}

void unlockFS() {
    if (syncStrategy == MUTEX) {
        pthread_mutex_unlock(&mutex);
    } else if (syncStrategy == RWLOCK) {
        pthread_rwlock_unlock(&rwlock);
    }
}

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if (syncStrategy != NOSYNC) {
        pthread_mutex_lock(&mutex);
    }

    if (numberCommands > 0) {
        numberCommands--;
        int head = headQueue++;

        if (syncStrategy != NOSYNC) {
            pthread_mutex_unlock(&mutex);
        }

        return inputCommands[head];
    }

    if (syncStrategy != NOSYNC) {
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void errorParse() {
    printf("Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(FILE *inputFile) {
    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), inputFile)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
}

void applyCommands() {
    while (numberCommands > 0) {
        const char* command = removeCommand();
        
        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        
        if (numTokens < 2) {
            printf("Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                            break;
                    default:
                        printf("Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }

                break;
            case 'l': 
                lockReadFS();
                searchResult = lookup(name);
                unlockFS();

                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;

            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;
            default: { /* error */
                printf("Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void *threadFunction(void *arg) {
    applyCommands();

    return NULL;
}

int main(int argc, char* argv[]) {
    struct timeval tv1, tv2;
    FILE *inputFile, *outputFile;
    
    checkError(argc != 5, "wrong number of arguments");

    if (!strcmp(argv[4], "mutex")) {
        syncStrategy = MUTEX;
    } else if (!strcmp(argv[4], "rwlock")) {
        checkError(pthread_rwlock_init(&rwlock, NULL), "RWLock failed to initialize");
        syncStrategy = RWLOCK;
    } else if (!strcmp(argv[4], "nosync")) {
        syncStrategy = NOSYNC;
    } else {
        printf("Error: Invalid sync strategy.\n");
        exit(EXIT_FAILURE);
    }

    if (syncStrategy != NOSYNC) {
        checkError(pthread_mutex_init(&mutex, NULL), "Mutex failed to initialize");
    }
    
    /* init filesystem */
    init_fs();

    /* open input file */
    inputFile = fopen(argv[1], "r");
    checkError(!inputFile, "could not open input file");

    /* process input */
    processInput(inputFile);
    fclose(inputFile);

    /* create tasks pool */
    numberThreads = atoi(argv[3]);
    pthread_t tid[numberThreads];
    
    gettimeofday(&tv1, NULL);

    for(int i = 0; i < numberThreads; i++) {
        if (pthread_create(&tid[i], NULL, threadFunction, NULL) != 0) {
            printf("Error: could not create thread\n");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < numberThreads; i++) {
        if (pthread_join(tid[i], NULL)) {
            printf("Error: error waiting for thread\n");
            exit(EXIT_FAILURE);
        }
    }

    gettimeofday(&tv2, NULL);
    printf("TecnicoFS completed in %.4f seconds\n", 
        (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
        (double) (tv2.tv_sec - tv1.tv_sec));

    outputFile = fopen(argv[2], "w");
    if (!outputFile) {
        printf("Error: could not open output file\n");
        exit(EXIT_FAILURE);
    }

    /* print tree */
    print_tecnicofs_tree(outputFile);
    fclose(outputFile);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}
