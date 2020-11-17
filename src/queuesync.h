#ifndef QUEUE_SYNC_H
#define QUEUE_SYNC_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void init_sync();
void destroy_sync();
void lock_command_queue();
void unlock_command_queue();
void wait_for_consumer();
void wait_for_producer();
void signal_producer();
void signal_consumer();

#endif /* QUEUE_SYNC_H */
