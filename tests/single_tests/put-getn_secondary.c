#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mpi.h"
#include "mdhim.h"

#define SECONDARY_SLICE_SIZE 6

int main(int argc, char **argv) {
	int ret;
	int provided = 0;
	struct mdhim_t *md;
	uint32_t key, **secondary_keys;
	int value, *secondary_key_lens;
	struct mdhim_brm_t *brm;
	struct mdhim_bgetrm_t *bgrm;
	int i;
	int keys_per_rank = 100;
	char     *db_path = "./";
	char     *db_name = "mdhimTstDB-";
	int      dbug = MLOG_DBG;
	mdhim_options_t *db_opts; // Local variable for db create options to be passed
	int db_type = LEVELDB; //(data_store.h) 
	struct timeval start_tv, end_tv;
	unsigned totaltime;
	struct index_t *secondary_index;
	struct secondary_info *secondary_info;

	// Create options for DB initialization
	db_opts = mdhim_options_init();
	mdhim_options_set_db_path(db_opts, db_path);
	mdhim_options_set_db_name(db_opts, db_name);
	mdhim_options_set_db_type(db_opts, db_type);
	mdhim_options_set_key_type(db_opts, MDHIM_INT_KEY);
	mdhim_options_set_debug_level(db_opts, dbug);

	gettimeofday(&start_tv, NULL);
	ret = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	if (ret != MPI_SUCCESS) {
		printf("Error initializing MPI with threads\n");
		exit(1);
	}

	if (provided != MPI_THREAD_MULTIPLE) {
                printf("Not able to enable MPI_THREAD_MULTIPLE mode\n");
                exit(1);
        }

	md = mdhimInit(MPI_COMM_WORLD, db_opts);
	if (!md) {
		printf("Error initializing MDHIM\n");
		exit(1);
	}	

	//Create the secondary remote index
	secondary_index = create_global_index(md, 2, SECONDARY_SLICE_SIZE, LEVELDB, 
					      MDHIM_INT_KEY, 
					      md->primary_index->id);	
	//Put the primary keys and values
	for (i = 0; i < keys_per_rank; i++) {
		key = keys_per_rank * md->mdhim_rank + i;
		value = md->mdhim_rank + i;
		//Create the secondary info struct
		secondary_keys = malloc(sizeof(uint32_t *));		
		secondary_keys[0] = malloc(sizeof(uint32_t));
		*secondary_keys[0] = md->mdhim_rank + i;
		secondary_key_lens = malloc(sizeof(int));
		secondary_key_lens[0] = sizeof(uint32_t);

		secondary_info = mdhimCreateSecondaryInfo(secondary_index,
							  (void **) secondary_keys, 
							  secondary_key_lens,
							  1, SECONDARY_GLOBAL_INFO);
		brm = mdhimPut(md,  
			       &key, sizeof(key), 
			       &value, sizeof(value), 
			       secondary_info, NULL);
		if (!brm || brm->error) {
			printf("Error inserting key/value into MDHIM\n");
		} else {
			printf("Rank: %d put secondary key: %u with value: %d\n", md->mdhim_rank, 
			       *secondary_keys[0], key);
		}

		mdhimReleaseSecondaryInfo(secondary_info);
		mdhim_full_release_msg(brm);
		free(secondary_keys[0]);
		free(secondary_keys);
		free(secondary_key_lens);
	}

	//Commit the database
	ret = mdhimCommit(md, md->primary_index);
	if (ret != MDHIM_SUCCESS) {
		printf("Error committing MDHIM database\n");
	} else {
		printf("Committed MDHIM database\n");
	}

	//Get the stats for the primary index
	ret = mdhimStatFlush(md, md->primary_index);
	if (ret != MDHIM_SUCCESS) {
		printf("Error getting stats\n");
	} else {
		printf("Got stats\n");
	}

	//Get the stats for the secondary index
	ret = mdhimStatFlush(md, secondary_index);
	if (ret != MDHIM_SUCCESS) {
		printf("Error getting stats\n");
	} else {
		printf("Got stats\n");
	}

	//Get the secondary keys and values using get_next
	for (i = 1; i < keys_per_rank; i++) {
		value = 0;
		key = md->mdhim_rank + i - 1;
		bgrm = mdhimBGetOp(md, secondary_index, 
				   &key, sizeof(uint32_t), 1, MDHIM_GET_NEXT);				
		if (!bgrm || bgrm->error) {
			printf("Rank: %d, Error getting next key/value given key: %d from MDHIM\n", 
			       md->mdhim_rank, key);
		} else if (bgrm->keys && bgrm->values) {
			printf("Rank: %d successfully got key: %d with value: %d from MDHIM\n", 
			       md->mdhim_rank,
			       *((int *) bgrm->keys[0]),
			       *((int *) bgrm->values[0]));
		}

		mdhim_full_release_msg(bgrm);
	}

	ret = mdhimClose(md);
	mdhim_options_destroy(db_opts);
	if (ret != MDHIM_SUCCESS) {
		printf("Error closing MDHIM\n");
	}

	gettimeofday(&end_tv, NULL);
	totaltime = end_tv.tv_sec - start_tv.tv_sec;
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	printf("Took %u seconds to insert and retrieve %d keys/values\n", totaltime, 
	       keys_per_rank);

	return 0;
}