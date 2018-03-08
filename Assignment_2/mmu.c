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
int pfhandled = 0;

/*
* Functions prototype
*/
void handle_pagefault_serviced();

int main(int argc, char const *argv[])
{
	int i;
	int shmid;
	pageTable *shm;

	if (argc < 5)
	{
		printf("Usage : <object file name> <Memory trace file> <No of page bits> <No of offset bits> <PID of OS>\n");
		exit(0);
	}

	/*
	* Create page table (shared memory)
	*/
	key_t key = 5678;
	if((shmid = shmget(key,sizeof(pageTable),IPC_CREAT | 0666)) < 0){
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
	* Capture signal from OS
	*/
	if(signal(SIGCONT,handle_pagefault_serviced) == SIG_ERR){
		printf("Pagefault service signal error..\n");
		exit(1);
	}
	
	/*
	* Handle CPU request i.e service all entry of memory trace file
	*/

	FILE *fp;
	char *cpurequest = (char *)malloc(sizeof(char)*100);
	if((fp = fopen(argv[1],"r")) != NULL){
		printf("File reading error..\n");
		exit(1);
	}
	while(getline(&cpurequest,0,fp) != -1){


		/*
		* Process a cpu request line and get page no 
		* Format: 0x105ff0 W
		*/


	}

	return 0;
}

void handle_pagefault_serviced(){
	pfhandled = 1;
}