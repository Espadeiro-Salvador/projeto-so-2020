#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>
#include "fs/operations.h" 

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int insptr = 0, remptr = 0;
int headQueue = 0;

pthread_mutex_t mutex;
pthread_cond_t canInsert, canRemove;

void insertCommand(char* data) {
    if (pthread_mutex_lock(&mutex)) {
        printf("Error: Mutex failed to lock\n");
        exit(EXIT_FAILURE);
    }
    
    while (numberCommands == MAX_COMMANDS) {
        pthread_cond_wait(&canInsert, &mutex);
    }

    strcpy(inputCommands[insptr], data);

    insptr++;
    if (insptr == MAX_COMMANDS) {
        insptr = 0;
    }
    
    numberCommands++;

    pthread_cond_signal(&canRemove);
    pthread_mutex_unlock(&mutex);
}

char *removeCommand() {
    char command[MAX_INPUT_SIZE];

    if (pthread_mutex_lock(&mutex)) {
        printf("Error: Mutex failed to lock\n");
        exit(EXIT_FAILURE);
    }
    
    while (numberCommands == 0) {
        pthread_cond_wait(&canRemove, &mutex);
    }
    
    strcpy(command, inputCommands[remptr]);
    
    remptr++;
    if (remptr == MAX_COMMANDS) {
        remptr = 0;
    }
        
    numberCommands--;
    
    pthread_cond_signal(&canInsert);
    pthread_mutex_unlock(&mutex);
    
    return command;
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
                insertCommand(line);
                break;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                insertCommand(line);
                break;
            
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                insertCommand(line);
                break;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
}

void applyCommands() {
    while (1) {
        if (numberCommands <= 0) {
            break;
        }

        const char* command = removeCommand();
        
        if (command == NULL) {
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
                searchResult = lookup(name);

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
    
    if (argc != 4) {
        printf("Error: wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_mutex_init(&mutex, NULL)) {
        printf("Error: Mutex failed to init\n");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_cond_init(&canInsert, NULL)) {
        printf("Error: Cond failed to init\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&canRemove, NULL)) {
        printf("Error: Cond failed to init\n");
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

    /* close input file */
    if (fclose(inputFile)) {
        printf("Error: could not close the input file\n");
        exit(EXIT_FAILURE);
    }

    numberThreads = atoi(argv[3]);

    if (numberThreads < 1) {
        printf("Error: can't run less than one thread\n");
        exit(EXIT_FAILURE);
    }

    pthread_t tid[numberThreads];

    /* create the tasks */
    for(int i = 0; i < numberThreads; i++) {
        if (pthread_create(&tid[i], NULL, threadFunction, NULL) != 0) {
            printf("Error: could not create thread\n");
            exit(EXIT_FAILURE);
        }
    }

    /* get initial time */
    if (gettimeofday(&tv1, NULL) == -1) {
        printf("Error: could not get the time\n");
        exit(EXIT_FAILURE);
    }

    /* process input */
    processInput(inputFile);

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

    if (pthread_mutex_destroy(&mutex)) {
        printf("Error: Mutex failed to destroy\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_destroy(&canInsert)) {
        printf("Error: Cond failed to destroy\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_destroy(&canRemove)) {
        printf("Error: Cond failed to destroy\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
