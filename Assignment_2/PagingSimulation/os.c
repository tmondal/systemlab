#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>
#include <limits.h>

typedef struct {
	
	bool valid;
	int fno;
	bool dirty;
	int requested;
	time_t last_referenced;
}pageTable;


typedef struct
{
	time_t filledAt;
	int assigned_pageno; 
}FRAME;

/*
* Global variables declaration
*/
int pfhandle = 0;
int quit = 0;
int disk_access = 0;

/*
* Functions prototype
*/
void handle_pagefault();
void handle_quit();

void call_fifo(pageTable *shm, FRAME *frame,int i, int noofframes, int noofpages){
	
	printf("FIFO invoked..\n");
	int k;
	time_t min = frame[0].filledAt;
	int minindex = 0;
	for (k = 1; k < noofframes; ++k)
	{
		if (frame[k].filledAt < min)
		{
			min = frame[k].filledAt;
			minindex = k; // victim frame
		}
	}
	printf("Found !! victim frame is : frame[%d] --> pageno[%d]\n",minindex,frame[minindex].assigned_pageno);
	/*
	* Replace page 
	*/
	if (frame[minindex].assigned_pageno < noofpages && frame[minindex].assigned_pageno >= 0)
	{
		/*
		* If not dirty bring page from HDD 
		* else write back victim page then bring new page
		*/
		if (shm[frame[minindex].assigned_pageno].dirty)
		{
			printf("Writing back to HDD...\n");
			sleep(4); // writing back 
			disk_access++; // written back the dirty page
			printf("Written back successfully..\n");
		}
		shm[frame[minindex].assigned_pageno].fno = -1; // snatched
		shm[frame[minindex].assigned_pageno].valid = false;
		shm[frame[minindex].assigned_pageno].dirty = false;
		printf("Bringing requested page from HDD...\n");
		sleep(4);
		disk_access++; // since bringing from HDD
		frame[minindex].assigned_pageno = i;       // replaced by page[i]
		sleep(1);
		frame[minindex].filledAt = time(0);

		shm[i].fno = minindex;
		shm[i].valid = true;
		printf("FIFO has completed page replacement..\n");
	}
	else{
		printf("Something wrong with pageno. Couldn't replace.\n");
	}
}

void call_lru(pageTable *shm, FRAME *frame,int new_pno, int noofpages){
	printf("LRU invoked..\n");
	printf("New pno: %d\n", new_pno);
	int k;
	/*
	* Search the page that referenced least recently and valid
	*/
	time_t min = time(0);
	int minindex;
	for (k = 0; k < noofpages; ++k)
	{
		printf("page[%d] referenced at: %ld validity : %d\n",k, (long)shm[k].last_referenced,shm[k].valid);
		if (shm[k].last_referenced < min && shm[k].valid)   // last accessed and must be valid
		{
			min = shm[k].last_referenced;
			minindex = k; // victim page
		}
	}
	/*
	* Replace victim page with requested page 
	*/
	printf("Victim page: %d\n", minindex);
	if (shm[minindex].dirty)
	{
		printf("Writing back to HDD...\n");
		sleep(4);
		disk_access++;
		printf("Written back successfully..\n");
	}

	printf("Bringing requested page from HDD...\n");
	sleep(4);
	disk_access++;

	shm[new_pno].fno = shm[minindex].fno; // Winner
	shm[new_pno].valid = true;
	sleep(1);
	shm[new_pno].last_referenced = time(0);

	shm[minindex].fno = -1; // snatched
	shm[minindex].valid = false;
	shm[minindex].dirty = false;

	frame[shm[minindex].fno].assigned_pageno = new_pno;
	frame[shm[minindex].fno].filledAt = shm[new_pno].last_referenced;
	printf("LRU completed page replacement..\n"); 
}

void main(int argc, char const *argv[])
{
	int i,pno,fno;
	int shmid,mmupid;
	pageTable *shm;

	if (argc < 3)
	{
		printf("Usage : <object file name> <No of pages> <No of Frames>\n");
		exit(0);
	}

	int noofpages = atoi(argv[1]);
	int noofframes = atoi(argv[2]);
	
	/*
	* Create RAM
	*/
	FRAME *frame = (FRAME *)malloc(sizeof(FRAME)*noofframes);
	for (i = 0; i < noofframes; ++i)
	{
		frame[i].filledAt = time(0);
		frame[i].assigned_pageno = -1;
	}

	/*
	* Create page table (shared memory)
	* ftok(path,int) is used to generate a key 
	* All the files of the current folder that uses "."" as path
	* will get same key if next int (here it is 'x') is same 
	*/

	key_t key = ftok(".",'x');
	if((shmid = shmget(key,noofpages*sizeof(pageTable),IPC_CREAT | 0666)) < 0){
		printf("Error creating shared memory..\n");
		exit(1);
	}
	else{
		printf("Page table created.\n");
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
	for (i = 0; i < noofpages; ++i)
	{
		shm[i].valid = false;
		shm[i].dirty = false;
		shm[i].requested = 0;
		shm[i].fno = -1;
		shm[i].last_referenced = time(0);
	}
	printf("Page table initialized.\n");
	printf("OS pid : %d\n", getpid());
	printf("Waiting for pagefault singal from MMU...\n");
	/*
	* Capture signal from MMU
	*/
	if(signal(SIGUSR1,handle_pagefault) == SIG_ERR){
		printf("Pagefault signal error..\n");
		exit(1);
	}
	if(signal(SIGUSR2,handle_quit) == SIG_ERR){
		printf("Quit signal error..\n");
		exit(1);
	}

	/*
	* Service pagefault once get signal from MMU
	*/
	while(1){

		if (pfhandle)
		{
			/*
			* Scan the page table and search for non-zero entry of requested field.
			* Then service appropriate page
			*/
			for (pno = 0; pno < noofpages; ++pno)
			{
				if (shm[pno].requested)
				{
					/*
					* Find free frame if exists and populate by requested page 
					*/
					printf("Requested pno: %d\n", pno);
					for (fno = 0; fno < noofframes;)
					{
						if (frame[fno].assigned_pageno >= 0 && frame[fno].assigned_pageno < noofpages)
						{
							fno++;
						}
						else{
							sleep(1);
							shm[pno].fno = fno;
							shm[pno].last_referenced = time(0) % 10000;
							frame[fno].assigned_pageno = pno;
							disk_access++; 
							break;
						}
					}
					/*
					* If free frame not found , use FIFO or LRU replacement algorithm
					*/
					if (fno == noofframes)
					{
						call_fifo(shm,frame,pno,noofframes,noofpages);
						// call_lru(shm,frame,pno,noofpages);
					}

					/*
					* Once page fault serviced , send signal to wake up MMU
					*/
					printf("MMU pid: %d\n", shm[pno].requested);
					mmupid = shm[pno].requested;
					kill(shm[pno].requested,SIGCONT);
					shm[pno].dirty = false;
					shm[pno].valid = true;
					shm[pno].requested = 0;
					pfhandle = 0;
					break;
				}

			}
		}
		if (quit)
			break;
	}
	printf("No of disk access: %d\n",disk_access);
	/*
	* Detach shm and destroy shmid i.e page table
	*/	
	shmdt(shm);
	shmctl(shmid,IPC_RMID,NULL);
	exit(0);
}

void handle_pagefault(){
	pfhandle = 1;
}

void handle_quit(){
	quit = 1;
}