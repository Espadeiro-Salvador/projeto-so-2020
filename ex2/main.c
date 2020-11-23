#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>
#include "fs/operations.h" 
#include "queuesync.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int insptr = 0, remptr = 0;

void insertCommand(char* data) {
    lock_command_queue();
    
    while (numberCommands == MAX_COMMANDS) {
        wait_for_consumer();
    }

    strcpy(inputCommands[insptr], data);

    insptr++;
    if (insptr == MAX_COMMANDS) {
        insptr = 0;
    }
    
    numberCommands++;

    signal_consumer();
    unlock_command_queue();
}

void removeCommand(char *command) {
    lock_command_queue();
    
    while (numberCommands == 0) {
        wait_for_producer();
    }
    
    strcpy(command, inputCommands[remptr]);
    
    remptr++;
    if (remptr == MAX_COMMANDS) {
        remptr = 0;
    }
        
    numberCommands--;
    
    signal_producer();
    unlock_command_queue();
}

void errorParse(const char *command) {
    printf("Error: command invalid: %s\n", command);
    exit(EXIT_FAILURE);
}

void processInput(FILE *inputFile) {
    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), inputFile)) {
        char token;
        char arg1[MAX_INPUT_SIZE];
        char arg2[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %s", &token, arg1, arg2);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }

        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse(line);
                insertCommand(line);
                break;
            
            case 'l':
                if(numTokens != 2)
                    errorParse(line);
                insertCommand(line);
                break;
            
            
            case 'd':
                if(numTokens != 2)
                    errorParse(line);
                insertCommand(line);
                break;
            
            case 'm':
                 if(numTokens != 3)
                    errorParse(line);
                insertCommand(line);
                break;
            case '#':
                break;
            
            default: { /* error */
                errorParse(line);
            }
        }
    }

    char command[1];
    command[0] = EOF;
    for (int i = 0; i < numberThreads; i++) {
        insertCommand(command);   
    }
}

void applyCommands() {
    while (1) {
        char command[MAX_INPUT_SIZE];
        removeCommand(command);
        
        if (command == NULL) {
            continue;
        }

        char token;
        char arg1[MAX_INPUT_SIZE];
        char arg2[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %s", &token, arg1, arg2);

        if (token == EOF) return;

        if (numTokens < 2) {
            printf("Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (arg2[0]) {
                    case 'f':
                        printf("Create file: %s\n", arg1);
                        create(arg1, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", arg1);
                        create(arg1, T_DIRECTORY);
                        break;
                    default:
                        printf("Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup(arg1);

                if (searchResult >= 0)
                    printf("Search: %s found\n", arg1);
                else
                    printf("Search: %s not found\n", arg1);
                break;

            case 'd':
                printf("Delete: %s\n", arg1);
                delete(arg1);
                break;

            case 'm':
                printf("Move: %s to %s\n", arg1, arg2);
                move(arg1, arg2);
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

/*
 * Starts the timer
 * Input:
 *  - start: pointer to the start of the timer
 */
void start_timer(struct timeval *start) {
    if (gettimeofday(start, NULL) == -1) {
        printf("Error: could not get the time\n");
        exit(EXIT_FAILURE);
    }
}

/*
 * Stops the timer
 * Input:
 *  - start: pointer to the start of the timer
 * Returns: duration of the timer
 */
double stop_timer(struct timeval *start) {
    struct timeval stop;

    /* get final time */
    if (gettimeofday(&stop, NULL) == -1) {
        printf("Error: could not get the time\n");
        exit(EXIT_FAILURE);
    }

    return (double) (stop.tv_usec - start->tv_usec) / 1000000 + 
        (double) (stop.tv_sec - start->tv_sec);
}

/*
 * Creates the number of threads given
*/
void create_thread_pool(pthread_t *tid, int numberThreads) {
    for(int i = 0; i < numberThreads; i++) {
        if (pthread_create(&tid[i], NULL, threadFunction, NULL) != 0) {
            printf("Error: could not create thread\n");
            exit(EXIT_FAILURE);
        }
    }
}

/*
 * Wait for the all the threads
 */
void wait_for_threads(pthread_t *tid, int numberThreads) {
    for (int i = 0; i < numberThreads; i++) {
        if (pthread_join(tid[i], NULL)) {
            printf("Error: error waiting for thread\n");
            exit(EXIT_FAILURE);
        }
    }
}

/*
 * Prints the TecnicoFS tree in the output file.
 */
void print_tree(char *name) {
    FILE *outputFile;

    /* open output file */
    outputFile = fopen(name, "w");
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
}

/*
 * Validates the number of threads and the number of args 
 */
void parse_args(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Error: wrong number of arguments\n");
        exit(EXIT_FAILURE);
    }

    /* get number of threads */
    numberThreads = atoi(argv[3]);
    if (numberThreads < 1) {
        printf("Error: can't run less than one thread\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    struct timeval start;
    FILE *inputFile;

    parse_args(argc, argv);
    
    /* init global mutex and conds */
    init_sync();
    /* init filesystem */
    init_fs();

    /* create the tasks */
    pthread_t tid[numberThreads];
    create_thread_pool(tid, numberThreads);

    /* open input file */
    inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        printf("Error: could not open the input file\n");
        exit(EXIT_FAILURE);
    }

    start_timer(&start);
    processInput(inputFile);

    /* close input file */
    if (fclose(inputFile)) {
        printf("Error: could not close the input file\n");
        exit(EXIT_FAILURE);
    }

    /* wait for all the threads to finish running */
    wait_for_threads(tid, numberThreads);
    printf("TecnicoFS completed in %.4f seconds.\n", stop_timer(&start));
    print_tree(argv[2]);

    /* release allocated memory */
    destroy_fs();
    destroy_sync();

    exit(EXIT_SUCCESS);
}
