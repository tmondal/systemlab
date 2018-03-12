#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <math.h>
#include <wait.h>

typedef struct {
	
	bool valid;
	int fno;
	bool dirty;
	int requested;
	time_t last_referenced;
}pageTable;

/*
* Global variables declaration
*/
int pfhandled = 0;

/*
* Functions prototype
*/
void handle_pagefault_serviced();

void main(int argc, char const *argv[])
{
	int i,p;
	int shmid;
	int pageno,page_hit = 0,page_fault = 0;
	char mode;
	pageTable *shm;

	if (argc < 6)
	{
		printf("Usage : <object file name> <No of pages > <Memory trace file> <No of page bits> <No of offset bits> <PID of OS>\n");
		exit(0);
	}

	int noofpages = atoi(argv[1]);
	int pagebits = atoi(argv[3]);
	int offset = atoi(argv[4]);
	int ospid = atoi(argv[5]);
	int addresslen = (pagebits + offset)/4;

	int temp = pow(2,pagebits);
	if (temp != noofpages)
	{
		printf("Missmacth between No of pages and page bits\n");
		exit(0);
	}

	/*
	* Create page table (shared memory)
	*/
	key_t key = ftok(".",'x');
	if((shmid = shmget(key,noofpages*sizeof(pageTable),IPC_CREAT | 0666)) < 0){
		printf("Error creating shared memory..\n");
		exit(0);
	}

	/*
	* Now attach current process to the aquired shared memory
	*/

	if ((shm = (pageTable *)shmat(shmid,NULL,0)) == (void *)-1)
	{
		printf("shmat error..\n");
		exit(0);
	}

	
	/*
	* Capture signal from OS
	*/
	if(signal(SIGCONT,handle_pagefault_serviced) == SIG_ERR){
		printf("Pagefault service signal error..\n");
		exit(0);
	}
	
	/*
	* Handle CPU request i.e service all entry of memory trace file
	*/

	FILE *fp;
	char *cpurequest = (char *)malloc(sizeof(char)*(addresslen + 2));
	if((fp = fopen(argv[2],"r")) == NULL){
		printf("File reading error..\n");
		exit(0);
	}
	while((fscanf(fp,"%s %c",cpurequest,&mode) != EOF)){


		/*
		* Process a cpu request line and get page no 
		* Format: 0x105ff0 W
		*/

		pageno = strtol(cpurequest,NULL,16)/pow(2,offset);
		printf("\nProcessed page no: %d\n", pageno);
		if (pageno < pow(2,pagebits) && pageno >= 0)
		{
			/*
			* If page present increament Page_hit else populate requested_field
			* of corresponding page with mmu pid
			*/
			if (!shm[pageno].valid)
			{
				page_fault++; 
				shm[pageno].requested = getpid();
				/*
				* If pagefault then send SIGUSER1 signal to OS and SIGSTOP 
				* until OS sends SIGCONT
				*/
				printf("OS pid: %d\n", ospid);
				printf("MMU pid: %d\n", getpid());
				kill(ospid,SIGUSR1);
				kill(getpid(),SIGSTOP);
			}
			else{
				sleep(1);
				shm[pageno].last_referenced = time(0);
				printf("\npage[%d] referenced at: %ld\n",pageno, (long)shm[pageno].last_referenced);
				page_hit++;
			}
			/*
			* If mode is W then set dirty bit
			*/
			if (mode == 'w' || mode == 'W')
			{
				shm[pageno].dirty = true;
			}
			
		}
		else{
			printf("Bad address..\n");
		}
		/*
		* Show updated pagetable
		*/
		printf("No of page hit: %d\n", page_hit);
		printf("No of page fault: %d\n",page_fault);
		printf("  -----------------------------------------------------------------\n");
		printf("  |                                                               |\n");
		printf("  |                          Page Table                           |\n");
		printf("  |                                                               |\n");
		printf("  -----------------------------------------------------------------\n");
		for (p = 0; p < noofpages; ++p)
		{
			if (shm[p].dirty != 0)
			{
				printf("\t-----------------------------------------------------\n");
				printf("\t|\n");
				printf("\t|page[%d]: Valid = %d Frame = %d Dirty = %d Requested = %d\n",p,shm[p].valid,shm[p].fno,shm[p].dirty,shm[p].requested);
				printf("\t|\n");
				printf("\t-----------------------------------------------------\n");
			}
			else
				printf("\tpage[%d]: Valid = %d Frame = %d Dirty = %d Requested = %d\n",p,shm[p].valid,shm[p].fno,shm[p].dirty,shm[p].requested);
		}
	}
	/*
	* Once all cpu request serviced detach from shared memory (page table)
	* then send signal to OS 
	*/
	shmdt(shm);
	kill(ospid,SIGUSR2);
	printf("MMU sent last signal to OS..\n");
	exit(0);
}

void handle_pagefault_serviced(){}