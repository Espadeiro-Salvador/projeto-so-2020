#ifndef PRINT_SYNC_H
#define PRINT_SYNC_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void init_printsync();
void destroy_printsync();
void printsync_lock();
void printsync_unlock();
void wait_for_print_task();
void wait_for_modifying_tasks();
void broadcast_modifying_tasks();
void signal_print_task();

#endif /* PRINT_SYNC_H */
