#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

typedef struct {
	
	bool valid;
	int fno;
	bool dirty;
	int requested;
}pageTable;

/*
* Global variables declaration
*/
int pfhandle = 0;
int quit = 0;

/*
* Functions prototype
*/
void handle_pagefault();
void handle_quit();

int main(int argc, char const *argv[])
{
	int i;
	int shmid;
	pageTable *shm;

	if (argc < 3)
	{
		printf("Usage : <object file name> <No of pages> <No of Frames>\n");
		exit(0);
	}

	/*
	* Create page table (shared memory)
	*/
	key_t key = 5678;
	if((shmid = shmget(key,(*argv[1])*sizeof(pageTable),IPC_CREAT | 0666)) < 0){
		printf("Error creating shared memory..\n");
		exit(0);
	}

	/*
	* Now attach current process to the aquired shared memory
	*/

	if ((shm = (pageTable *)shmat(shmid,NULL,0)) == (void *)-1)
	{
		printf("shmat error..\n");
		exit(1);
	}

	/*
	* Initialize shared memory
	*/
	for (i = 0; i < *argv[1]; ++i)
	{
		shm[i].valid = false;
		shm[i].dirty = false;
		shm[i].requested = 0;
		shm[i].fno = -1;
	}

	/*
	* Capture signal from MMU
	*/
	if(signal(SIGUSR1,handle_pagefault) == SIG_ERR){
		printf("Pagefault signal error..\n");
		exit(2);
	}
	if(signal(SIGUSR2,handle_quit) == SIG_ERR){
		printf("Quit signal error..\n");
		exit(1);
	}

	/*
	* Service pagefault once get signal from MMU
	*/
	while(!quit){

		if (pfhandle)
		{
			/*
			* Scan the page table and search for non-zero entry of requested field.
			* Then service appropriate page
			*/

			/*
			* Once page fault serviced , send signal to wake up MMU
			*/
		}
	}


	return 0;
}

void handle_pagefault(){
	pfhandle = 1;
}

void handle_quit(){
	quit = 1;
}