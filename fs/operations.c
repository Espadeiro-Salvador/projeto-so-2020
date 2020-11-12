#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;

}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY, NULL);

	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}


int getinumber(char *name, lockstack_t *lockstack) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;
	
	/* use for copy */
	type nType;
	union Data data;

	char *path = strtok_r(full_path, delim, &saveptr);

	/* get root inode data */
	if (path == NULL) {
		inode_get(current_inumber, &nType, &data, WRITE_LOCK, lockstack);
	} else {
		inode_get(current_inumber, &nType, &data, READ_LOCK, lockstack);
	}

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &saveptr);
		if (path == NULL) {
			inode_get(current_inumber, &nType, &data, WRITE_LOCK, lockstack);
		} else {
			inode_get(current_inumber, &nType, &data, READ_LOCK, lockstack);
		}
	}
	return current_inumber;
}

/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	lockstack_t lockstack;
	lockstack_init(&lockstack);

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);
	
	parent_inumber = getinumber(parent_name, &lockstack);

	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		lockstack_clear(&lockstack);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata, NO_LOCK, &lockstack);

	if (pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		lockstack_clear(&lockstack);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		lockstack_clear(&lockstack);
		return FAIL;
	}
	
	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType, &lockstack);
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		lockstack_clear(&lockstack);
		return FAIL;
	}

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		lockstack_clear(&lockstack);
		return FAIL;
	}

	lockstack_clear(&lockstack);
	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){
	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;
	
	lockstack_t lockstack;
	lockstack_init(&lockstack);

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = getinumber(parent_name, &lockstack);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		lockstack_clear(&lockstack);
		return FAIL;
	}

	/* parent is already locked for writing */
	inode_get(parent_inumber, &pType, &pdata, NO_LOCK, &lockstack);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		lockstack_clear(&lockstack);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		lockstack_clear(&lockstack);
		return FAIL;
	}

	inode_get(child_inumber, &cType, &cdata, WRITE_LOCK, &lockstack);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		lockstack_clear(&lockstack);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		lockstack_clear(&lockstack);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		lockstack_clear(&lockstack);
		return FAIL;
	}
	lockstack_clear(&lockstack);
	return SUCCESS;
}

/*
 * Moves node to a different path.
 * Input:
 *  - from: path of node
 * 	- to: path to move node into 
 * Returns:
 *  Returns: SUCCESS or FAIL
 */
void move(char *from, char *to) {
	int parent_inumber_from, parent_inumber_to, child_inumber;
	char name_copy[MAX_FILE_NAME];
	char *parent_name_from, *parent_name_to, *child_name;

	lockstack_t lockstack;
	lockstack_init(&lockstack);
	
	strcpy(name_copy, from);
	split_parent_child_from_path(name_copy, &parent_name_from, &child_name);
	strcpy(name_copy, to);
	split_parent_child_from_path(name_copy, &parent_name_to, &child_name);
	
	int compare = strcmp(parent_name_from, parent_name_to);
	
	if (compare == 0) {
		parent_inumber_from = getinumber(parent_name_from, &lockstack);
		if (parent_inumber_from == FAIL) {
			printf("failed to move %s, invalid parent dir %s\n",
					child_name, parent_name_from);
			lockstack_clear(&lockstack);
			return FAIL;
		}

		parent_inumber_to = parent_inumber_from;
	} else if (compare < 0) {
		parent_inumber_from = getinumber(parent_name_from, &lockstack);
		if (parent_inumber_from == FAIL) {
			printf("failed to move %s, invalid parent dir %s\n",
					child_name, parent_name_from);
			lockstack_clear(&lockstack);
			return FAIL;
		}

		parent_inumber_to = getinumber(parent_name_to, &lockstack);
		if (parent_inumber_to == FAIL) {
			printf("failed to move %s, invalid parent dir %s\n",
					child_name, parent_name_to);
			lockstack_clear(&lockstack);
			return FAIL;
		}
	} else {
		parent_inumber_to = getinumber(parent_name_to, &lockstack);
		if (parent_inumber_to == FAIL) {
			printf("failed to move %s, invalid parent dir %s\n",
					child_name, parent_name_to);
			lockstack_clear(&lockstack);
			return FAIL;
		}

		parent_inumber_from = getinumber(parent_name_from, &lockstack);
		if (parent_inumber_from == FAIL) {
			printf("failed to move %s, invalid parent dir %s\n",
					child_name, parent_name_from);
			lockstack_clear(&lockstack);
			return FAIL;
		}
	}

	if (dir_add_entry(parent_inumber_to, child_inumber, child_name) == FAIL) {
		lockstack_clear(&lockstack);
		return FAIL;
	}

	if (dir_reset_entry(parent_inumber_from, child_inumber) == FAIL) {
		lockstack_clear(&lockstack);
		return FAIL;
	}

	lockstack_clear(&lockstack);
	return SUCCESS;
}

/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;

	lockstack_t lockstack; 
	lockstack_init(&lockstack);

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;
	
	/* use for copy */
	type nType;
	union Data data;

	/* get root inode data */
	inode_get(current_inumber, &nType, &data, READ_LOCK, &lockstack);
	
	char *path = strtok_r(full_path, delim, &saveptr);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		inode_get(current_inumber, &nType, &data, READ_LOCK, &lockstack);
		path = strtok_r(NULL, delim, &saveptr);
	}
	lockstack_clear(&lockstack);
	return current_inumber;
}


/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}
