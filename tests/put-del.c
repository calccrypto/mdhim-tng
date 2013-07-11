#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mdhim.h"

int main(int argc, char **argv) {
	int ret;
	int provided = 0;
	struct mdhim_t *md;
	int key;
	int value;
	struct mdhim_rm_t *rm;
	struct mdhim_getrm_t *grm;

	ret = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	if (ret != MPI_SUCCESS) {
		printf("Error initializing MPI with threads\n");
		exit(1);
	}

	if (provided != MPI_THREAD_MULTIPLE) {
                printf("Not able to enable MPI_THREAD_MULTIPLE mode\n");
                exit(1);
        }

	md = mdhimInit(MPI_COMM_WORLD);
	if (!md) {
		printf("Error initializing MDHIM\n");
		exit(1);
	}	

	//Put the keys and values
	key = 20 * (md->mdhim_rank + 1);
	value = 1000 * (md->mdhim_rank + 1);
	rm = mdhimPut(md, &key, sizeof(key), MDHIM_INT_KEY, 
		       &value, sizeof(value));
	if (!rm || rm->error) {
		printf("Error inserting key/value into MDHIM\n");
	} else {
		printf("Successfully inserted key/value into MDHIM\n");
	}

	rm = mdhimDelete(md, &key, sizeof(key), MDHIM_INT_KEY);
	if (!rm || rm->error) {
		printf("Error deleting key/value from MDHIM\n");
	} else {
		printf("Successfully deleted key/value into MDHIM\n");
	}

	//Get the values
	value = 0;
	grm = mdhimGet(md, &key, sizeof(key), MDHIM_INT_KEY);
	if (!grm || grm->error) {
		printf("Error getting value for key: %d from MDHIM\n", key);
	} else if (grm->value_len) {
		printf("Successfully got value: %d from MDHIM\n", *((int *) grm->value));
	}

	ret = mdhimClose(md);
	if (ret != MDHIM_SUCCESS) {
		printf("Error closing MDHIM\n");
	}

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	return 0;
}