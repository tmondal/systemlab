#include  <stdio.h>
#include  <stdlib.h>
#include <string.h>
#include  <unistd.h>
#include  <time.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include  <pthread.h>
#include  <sys/wait.h>
#include  <semaphore.h>

#define NOOFFLIGHTS 10
#define NOOFTHREADS 20
#define MAX 5

/*
* define mutex variable globally
*/
pthread_rwlock_t rwlock[NOOFFLIGHTS];

sem_t sem[MAX];
int flag = 1;

typedef struct {
	int query_time;
	char query_type[10];
	int flight_no;
	int thread_no;
	int no_of_seats;
	int valid;
}sharedTable;

sharedTable table[MAX];


typedef struct args
{
	int *shmptr;
	int threadId;
}thread_arg;



void* doUserFunction(void *a){

	int ch,i,no_of_seats;
	thread_arg *args = (thread_arg *)a;
	int bookedStatus[NOOFFLIGHTS] = {0};
	int min;
	while(flag){

		/*
		*If multiple queries exist for a particular user in the table , find minimun query_time query
		* for the corresponding user and let it execute that query   
		*/
		min = 200;
		for (i = 0; i < MAX; ++i)
		{
			if (table[i].valid == 1 && (table[i].thread_no == args->threadId))
			{
				if (table[i].query_time < min)
				{
					min = table[i].query_time;
				}
			}
		}
		for (i = 0; i < MAX; ++i)
		{
			if((table[i].valid == 1) && (table[i].thread_no == args->threadId) && min == table[i].query_time)
			{
				/*
				* Make sure main thread of execution can't modify this(i^th) line now
				*/
				sem_wait(&sem[i]);
				/*
				* Make i^th entry invalid so that new data can be updated by main thread of execution
				* after this query is executed
				*/
				table[i].valid = 0; 
				if (strcmp(table[i].query_type,"Inquiry") == 0)
				{

					/*
					* if any other thread is writing on same flight
					* then fail
					* else show flight status
					*/
					
					pthread_rwlock_rdlock(&rwlock[table[i].flight_no-1]);

					printf("------------------------------------------------------------------------\n");
					printf("|\n");
					printf("|\tFlightNo-----|-----Booked-----|-----Available\n");
					printf("|\t%4d %18d %18d\n",table[i].flight_no, 150 - args->shmptr[table[i].flight_no-1],args->shmptr[table[i].flight_no-1]);
					printf("|\n");
					printf("------------------------------------------------------------------------\n\n");

					pthread_rwlock_unlock(&rwlock[table[i].flight_no-1]);
				}
				else if (strcmp(table[i].query_type,"Book") == 0) 
				{
					char permision[10];
					while(1){
						/*
						* First ask current user how many seats to book for specified flight
						*/
						printf("Hey user [%d] ! How many seats do you want to book for Flight [%d] : ",table[i].thread_no,table[i].flight_no);
						scanf("%d",&no_of_seats);
						table[i].no_of_seats = no_of_seats;

						/*
						* check if any other thread is reading(Inquiry) or writing(Book or Cancel)
						* on the same flight then fail
						* else do booking but apply limit condition
						*/
						
						pthread_rwlock_rdlock(&rwlock[table[i].flight_no-1]);

						/*
						* Check if requested seats are between 2 and 5
						* If so then check if enough seats available
						* then book else take appropriate action
						*/
						if (table[i].no_of_seats >= 2 && table[i].no_of_seats <= 5)
						{
							if (args->shmptr[table[i].flight_no-1] >= table[i].no_of_seats)
							{
								args->shmptr[table[i].flight_no-1] -= table[i].no_of_seats;
								bookedStatus[table[i].flight_no-1] += table[i].no_of_seats;

								printf("\nBooked :: [%d] seats booked on flight no: [%d] by user: [%d]\n",table[i].no_of_seats,table[i].flight_no,table[i].thread_no);
								break;
							}
							else{
								printf("\nSorry :: Less than [%d] seats remaining on flight no [%d].\n",table[i].no_of_seats,table[i].flight_no-1);
								printf("Do you want to book less ? [Y/N] : ");
								scanf("%s",permision);
								if (strcmp(permision,"N") == 0 || strcmp(permision,"n") == 0 || strcmp(permision,"No") == 0 || strcmp(permision,"NO") == 0)
								{
									break;	
								}
							}
						}
						else{
							printf("\nHey user [%d] you can book seats between [2 to 5].\n",table[i].thread_no);
							printf("Do you want to book less ? [Y/N] : ");
							scanf("%s",permision);
							if (strcmp(permision,"N") == 0 || strcmp(permision,"n") == 0 || strcmp(permision,"No") == 0 || strcmp(permision,"NO") == 0)
							{
								break;	
							}
						}

						pthread_rwlock_unlock(&rwlock[table[i].flight_no-1]);
					}
				}
				else if (strcmp(table[i].query_type,"Cancel") == 0)
				{
					char permision[10];
					while(1){
						/*
						* First ask current user how many seats to cancel for specified flight
						*/
						printf("Hey user [%d] ! How many seats do you want to cancel for Flight [%d] : ",table[i].thread_no,table[i].flight_no);
						scanf("%d",&no_of_seats);
						table[i].no_of_seats = no_of_seats;

						/*
						* check if booked atleast a seat on this flight
						* check if any other thread is reading(Inquiry) of writing(Book or Cancel)
						* on the same flight then fail
						* else cancel booking
						*/	

						pthread_rwlock_rdlock(&rwlock[table[i].flight_no-1]);

						/*
						* First check if current user booked any seat on the given flight
						* If booked then check if requested no of seats for cancelling less than no of booked seats
						* if so then cancel 
						* else do proper action
						*/
						
						if (bookedStatus[table[i].flight_no-1] >= table[i].no_of_seats)
						{	
							
							args->shmptr[table[i].flight_no-1] += table[i].no_of_seats;
							bookedStatus[table[i].flight_no-1] -= table[i].no_of_seats;
							printf("\nCanceled :: [%d] seats canceled from flight no: [%d] by thread: %d\n",table[i].no_of_seats,table[i].flight_no,table[i].thread_no);
							break;

						}
						else{
							printf("\nUser [%d] ! You cann't cancel seats more than you booked on flight [%d].\n",table[i].thread_no,table[i].flight_no);
							printf("Do you want to cancel less ? [Y/N] : ");
							scanf("%s",permision);
							if (strcmp(permision,"N") == 0 || strcmp(permision,"n") == 0 || strcmp(permision,"No") == 0 || strcmp(permision,"NO") == 0)
							{
								break;	
							}
						}
						pthread_rwlock_unlock(&rwlock[table[i].flight_no-1]); // if no write lock you unlock since you locked
					}
				}
				else{
					printf("Wrong query type.\n");
				}

				sem_post(&sem[i]);
			}
		} // end of for 

	} // end of while

	pthread_exit(NULL);
}

void  main(int  argc, char *argv[])
{
	int    shmid,i,j;
	pid_t  pid;
	int *shmptr; // capacity of all flights that is shared to all thread

	/*
	* Initialize valid value
	*/
	for (i = 0; i < MAX; ++i)
	{
		table[i].valid = 0;
	}

	shmid = shmget(IPC_PRIVATE, sizeof(int)*NOOFFLIGHTS, IPC_CREAT | 0666);
	if (shmid < 0) {
	  printf("*** shmget error (server) ***\n");
	  exit(1);
	}
	printf("Server has received a shared memory ...\n");
	shmptr = (int *) shmat(shmid, NULL, 0);

	if (shmptr == (void *)-1) {
	  printf("*** shmat error (server) ***\n");
	  exit(1);
	}
	printf("Server has attached the shared memory...\n");
	// Initialize flights with its capacity
	for (i = 0; i < NOOFFLIGHTS; ++i){
		shmptr[i] = 150;
	}

	// Creat a new child process
	printf("Server is about to fork a child process...\n");
	pid = fork();
	if (pid < 0) {
	  printf("*** fork error (server) ***\n");
	  exit(1);
	}
	else if (pid == 0) {

		/*
		* set counter for how long server to run then shutdown
		*/
		time_t start = time(NULL);
		time_t end = time(NULL);

		for (i = 0; i < MAX; ++i){
			sem_init(&sem[i],0,1);
		}

		for (i = 0; i < NOOFFLIGHTS; ++i)
		{
			pthread_rwlock_init(&rwlock[i],NULL);
		}

		/*
		* Open given file from where maximum 5 entries will be fetched at a time to sharedTable
		*/

		FILE *fp = fopen("input.txt","r");

		/*
		* create 20 threads 
		*/		
		thread_arg *arg[NOOFTHREADS];
		pthread_t users[20];
		int user;

		for (i = 0; i < NOOFTHREADS; ++i)
		{
			arg[i] = (thread_arg *)malloc(sizeof(thread_arg));
			arg[i]->shmptr = shmptr;
			arg[i]->threadId = i+1;
			user = pthread_create(&users[i], NULL, doUserFunction, arg[i]);
			if (user){
	          printf("ERROR: return code from pthread_create() is %d\n", user);
	          exit(0);
	       }
		}


		/*
		* Keep on populating shared table taking value from given file
		*/
		int qtime,flight_no,thread_no,no_of_seats;
		char qtype[10];
		int count = 0,i,j;
		while(flag){
			if (end - start > 1)
			{
				flag = 0;
			}
			for (i = 0; i < MAX; ++i)
			{
				if (!table[i].valid)
				{
					sem_wait(&sem[i]);

					if(fscanf(fp,"%d %s %d %d",&qtime,qtype,&flight_no,&thread_no) != EOF){
						// if(strcmp(qtype,"Inquiry"))
						// 	fscanf(fp,"%d",&no_of_seats);
						table[i].valid = 1;
						table[i].query_time = qtime;
						strcpy(table[i].query_type,qtype);
						table[i].flight_no = flight_no;
						table[i].thread_no = thread_no;
					}
					else{
						sem_post(&sem[i]);
						break;
					}
					sem_post(&sem[i]);
					
				}
			}

			/*
			* If all row of shared file is invalid i.e all queries are executed
			* and no more queries remaining in input file then get out from while
			*/
			for (j = 0; j < MAX;)
			{
				if(!table[j].valid)
					j++;
			}
			if(j == MAX)
				flag = 0;
			end = time(NULL);			
		}
		
		/*
		* Wait till threads are complete before main child thread continues.
    	* Unless we wait if we run  exit which will terminate 
    	* the process and all threads before the threads completes execution. 
    	*/

    	for (i = 0; i < NOOFFLIGHTS; ++i)
    	{
    		pthread_rwlock_destroy(&rwlock[i]);
    	}
    	for (i = 0; i < NOOFTHREADS; ++i)
    	{
    		pthread_join(users[i],NULL);
    	}
    	
		exit(0); // return to parent process
	}

	wait(NULL);
	printf("Server has detected the completion of its child...\n");
	printf("\nFinal status of each flight: \n\n");
	for(i = 0; i < NOOFFLIGHTS; i++){
		printf("------------------------------------------------------------------------\n");
		printf("|\n");
		printf("|\tFlightNo-----|-----Booked-----|-----Available\n");
		printf("|\t%4d %18d %18d\n",i+1, 150 - shmptr[i],shmptr[i]);
		printf("|\n");
		printf("------------------------------------------------------------------------\n\n");
	}
	shmdt((void *) shmptr);
	printf("Server has detached its shared memory...\n");
	shmctl(shmid, IPC_RMID, NULL);
	printf("Server has removed its shared memory...\n");
	printf("Server exits...\n");
	exit(0);
}