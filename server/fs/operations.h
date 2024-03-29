#ifndef FS_H
#define FS_H

#include "state.h"
#include "lockstack.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name);
int move(char *from, char *to);
void print_tecnicofs_tree(FILE *fp);
int print_tree(char *outputfile);

#endif /* FS_H */
