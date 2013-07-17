/*
 * MDHIM TNG
 * 
 * Data store abstraction
 */

#ifndef      __STORE_H
#define      __STORE_H

#include "unqlite.h"

/* Storage Methods */
#define UNQLITE 1 //UNQLITE storage method
#define LEVELDB 2 //LEVELDB storage method

/* mdhim_store_t flags */
#define MDHIM_CREATE 1 //Implies read/write 
#define MDHIM_RDONLY 2
#define MDHIM_RDWR 3

struct mdhim_store_t;
struct mdhim_store_opts_t;


/* Function pointers for abstracting data stores */
typedef int (*mdhim_store_open_fn_t)(void **db_handle, char *path, int flags, 
				     struct mdhim_store_opts_t *mstore_opts);
typedef int (*mdhim_store_put_fn_t)(void *db_handle, void *key, int key_len, void *data, int32_t data_len, 
				    struct mdhim_store_opts_t *mstore_opts);
typedef int (*mdhim_store_get_fn_t)(void *db_handle, void *key, int key_len, void **data, int32_t *data_len, 
				    struct mdhim_store_opts_t *mstore_opts);
typedef int (*mdhim_store_get_next_fn_t)(void *db_handle, void **key, 
					 int *key_len, void **data, 
					 int32_t *data_len, 
					 struct mdhim_store_opts_t *mstore_opts);
typedef int (*mdhim_store_get_prev_fn_t)(void *db_handle, void **key, 
					 int *key_len, void **data, 
					 int32_t *data_len, 
					 struct mdhim_store_opts_t *mstore_opts);
typedef int (*mdhim_store_del_fn_t)(void *db_handle, void *key, int key_len,
				    struct mdhim_store_opts_t *mstore_opts);
typedef int (*mdhim_store_commit_fn_t)(void *db_handle);
typedef int (*mdhim_store_close_fn_t)(void *db_handle, 
				      struct mdhim_store_opts_t *mstore_opts);

/* Generic mdhim storage object */
struct mdhim_store_t {
	int type;
	//handle to db
	void *db_handle;
	//Generic pointers 
	void *db_ptr1;
	void *db_ptr2;
	void *db_ptr3;
	void *db_ptr4;
	//Pointers to functions based on data store
	mdhim_store_open_fn_t open;
	mdhim_store_put_fn_t put;
	mdhim_store_get_fn_t get;
	mdhim_store_get_next_fn_t get_next;
	mdhim_store_get_prev_fn_t get_prev;
	mdhim_store_del_fn_t del;
	mdhim_store_commit_fn_t commit;
	mdhim_store_close_fn_t close;
};

/* mdhim storage options passed to direct storage access functions i.e.: get, put, open */
struct mdhim_store_opts_t {
	int key_type; 
	void *db_ptr1;
	void *db_ptr2;
	void *db_ptr3;
	void *db_ptr4;
};

//Initializes the data store based on the type given (i.e., UNQLITE, LEVELDB, etc...)
struct mdhim_store_t *mdhim_db_init(int db_type);
#endif
