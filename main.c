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

/* Locks using mutex or wrlock. */
void lockWriteFS() {
    if (syncStrategy == MUTEX && pthread_mutex_lock(&mutex)) {
        printf("Error: Mutex failed to lock\n");
        exit(EXIT_FAILURE);
    } else if (syncStrategy == RWLOCK && pthread_rwlock_wrlock(&rwlock)) {
        printf("Error: RWLock failed to lock\n");
        exit(EXIT_FAILURE);
    }
}

/* Locks using mutex or rdlock. */
void lockReadFS() {
    if (syncStrategy == MUTEX && pthread_mutex_lock(&mutex)) {
        printf("Error: Mutex failed to lock\n");
        exit(EXIT_FAILURE);
    } else if (syncStrategy == RWLOCK && pthread_rwlock_rdlock(&rwlock)) {
        printf("Error: RWLock failed to lock\n");
        exit(EXIT_FAILURE);   
    }
}

/* Unlocks the mutex or the rwlock. */
void unlockFS() {
    if (syncStrategy == MUTEX && pthread_mutex_unlock(&mutex)) {
        printf("Error: Mutex failed to unlock\n");
        exit(EXIT_FAILURE);
    } else if (syncStrategy == RWLOCK && pthread_rwlock_unlock(&rwlock)) {
        printf("Error: RWLock failed to unlock\n");
        exit(EXIT_FAILURE);
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
    if (syncStrategy != NOSYNC && pthread_mutex_lock(&mutex)) {
        printf("Error: Mutex failed to lock\n");
        exit(EXIT_FAILURE);
    }

    if (numberCommands > 0) {

        numberCommands--;
        int head = headQueue++;

        if (syncStrategy != NOSYNC && pthread_mutex_unlock(&mutex)) {
            printf("Error: Mutex failed to unlock\n");
            exit(EXIT_FAILURE);
        }

        return inputCommands[head];
    }

    if (syncStrategy != NOSYNC && pthread_mutex_unlock(&mutex)) {
        printf("Error: Mutex failed to unlock\n");
        exit(EXIT_FAILURE);
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
                searchResult = lookup(name, LOCK);

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
    
    if (argc != 5) {
        printf("Error: wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    /* select synchronization strategy */
    if (!strcmp(argv[4], "mutex")) {
        syncStrategy = MUTEX;
    } else if (!strcmp(argv[4], "rwlock")) {
        if (pthread_rwlock_init(&rwlock, NULL)) {
            printf("Error: RWLock failed to initialize\n");
            exit(EXIT_FAILURE);
        }
        syncStrategy = RWLOCK;
    } else if (!strcmp(argv[4], "nosync")) {
        syncStrategy = NOSYNC;
    } else {
        printf("Error: Invalid sync strategy\n");
        exit(EXIT_FAILURE);
    }

    if (syncStrategy != NOSYNC && pthread_mutex_init(&mutex, NULL)) {
        printf("Error: Mutex failed to initialize\n");
        exit(EXIT_FAILURE);
    }
    
    /* init filesystem */
    init_fs();

    /* open input file */
    inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        printf("Error: could not open the input file\n");
        exit(EXIT_FAILURE);
    }

    /* process input */
    processInput(inputFile);

    /* close input file */
    if (fclose(inputFile)) {
        printf("Error: could not close the input file\n");
        exit(EXIT_FAILURE);
    }

    numberThreads = atoi(argv[3]);
    pthread_t tid[numberThreads];
    
    /* get initial time */
    if (gettimeofday(&tv1, NULL) == -1) {
        printf("Error: could not get the time\n");
        exit(EXIT_FAILURE);
    }

    /* create the tasks */
    for(int i = 0; i < numberThreads; i++) {
        if (pthread_create(&tid[i], NULL, threadFunction, NULL) != 0) {
            printf("Error: could not create thread\n");
            exit(EXIT_FAILURE);
        }
    }

    /* wait for all the threads to finish running */
    for (int i = 0; i < numberThreads; i++) {
        if (pthread_join(tid[i], NULL)) {
            printf("Error: error waiting for thread\n");
            exit(EXIT_FAILURE);
        }
    }

    /* get final time */
    if (gettimeofday(&tv2, NULL) == -1) {
        printf("Error: could not get the time\n");
        exit(EXIT_FAILURE);
    }

    /* prints execution time */
    printf("TecnicoFS completed in %.4f seconds\n", 
        (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
        (double) (tv2.tv_sec - tv1.tv_sec));

    
    /* open output file */
    outputFile = fopen(argv[2], "w");
    if (!outputFile) {
        printf("Error: could not open output file\n");
        exit(EXIT_FAILURE);
    }

    /* print tree */
    print_tecnicofs_tree(outputFile);

    /* close output file */
    if (fclose(outputFile)) {
        printf("Error: could not close the output file\n");
        exit(EXIT_FAILURE);
    }

    /* release allocated memory */
    destroy_fs();
    if (syncStrategy != NOSYNC && pthread_mutex_destroy(&mutex)) {
        printf("Error: could not destroy the mutex\n");
        exit(EXIT_FAILURE);
    }
    if (syncStrategy == RWLOCK && pthread_rwlock_destroy(&rwlock)) {
        printf("Error: could not destroy the rwlock\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
