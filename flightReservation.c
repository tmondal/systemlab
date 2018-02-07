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
#include <semaphore.h>

#define NOOFFLIGHTS 10
#define NOOFTHREADS 20
#define MAX 5

/*
* define mutex variable globally
*/
pthread_mutex_t read_lock[NOOFFLIGHTS];
pthread_mutex_t write_lock[NOOFFLIGHTS];

sem_t sem[MAX];
int flag = 1;

// pthread_mutex_t mutex_thread;
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
	int bookedStatus[NOOFFLIGHTS];
}thread_arg;



void* doUserFunction(void *a){

	//printf("came..\n");
	int ch,i;
	thread_arg *args = (thread_arg *)a;
	printf("Thread_id: %d\n", args->threadId);

	while(flag){
		
		for (i = 0; i < MAX; ++i)
		{
			if((table[i].valid == 1) && (table[i].thread_no == args->threadId))
			{
				// Make sure main thread of execution can't modify this(i^th) line now
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
					pthread_mutex_lock(&write_lock[table[i].flight_no-1]); // check if any other user already locked to write
					pthread_mutex_unlock(&write_lock[table[i].flight_no-1]); // if no write lock you unlock since you locked
					pthread_mutex_lock(&read_lock[table[i].flight_no-1]); // if no write lock then lock as read

					printf("------------------------------------------------------------------------\n");
					printf("|\n");
					printf("|\tFlightNo-----|-----Booked-----|-----Available\n");
					printf("|\t%4d %18d %18d\n",table[i].flight_no, 150 - args->shmptr[table[i].flight_no-1],args->shmptr[table[i].flight_no-1]);
					printf("|\n");
					printf("------------------------------------------------------------------------\n\n");

					pthread_mutex_unlock(&read_lock[table[i].flight_no-1]); // finally unlock the read lock aquired
				}
				else if (strcmp(table[i].query_type,"Book") == 0) 
				{

					/*
					* check if already booked 
					* check if any other thread is reading(Inquiry) of writing(Book or Cancel)
					* on the same flight then fail
					* else do booking but apply limit condition
					*/
					pthread_mutex_lock(&write_lock[table[i].flight_no-1]); // check if any other user already locked to write
					pthread_mutex_lock(&read_lock[table[i].flight_no-1]); // if already read lock
					pthread_mutex_unlock(&read_lock[table[i].flight_no-1]); // if no read lock the unlock readlock

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
							args->bookedStatus[table[i].flight_no-1] = 1;
							printf("\nBooked :: [%d] seats booked on flight no: [%d] by user: [%d]\n",table[i].no_of_seats,table[i].flight_no,table[i].thread_no);
						}
						else
							printf("\nSorry :: Less than [%d] seats remaining on flight no [%d].\n",table[i].no_of_seats,table[i].flight_no);
					}
					else{
						printf("\nThread [%d] can book seats between [2 to 5].\n",table[i].thread_no);
					}
					pthread_mutex_unlock(&write_lock[table[i].flight_no-1]); // if no write lock you unlock since you locked
				}
				else if (strcmp(table[i].query_type,"Cancel") == 0)
				{

					/*
					* check if booked atleast a seat on this flight
					* check if any other thread is reading(Inquiry) of writing(Book or Cancel)
					* on the same flight then fail
					* else cancel booking
					*/	

					pthread_mutex_lock(&write_lock[table[i].flight_no-1]); // check if any other user already locked to write
					pthread_mutex_lock(&read_lock[table[i].flight_no-1]); // if already read lock
					pthread_mutex_unlock(&read_lock[table[i].flight_no-1]); // if no read lock the unlock readlock

					/*
					* First check if current user booked any seat on the given flight
					* If booked then check if requested no of seats for cancelling less than no of booked seats
					* if so then cancel 
					* else do proper action
					*/
					if (args->bookedStatus[table[i].flight_no-1] == 1)
					{	
						int bookedseat = 150 - args->shmptr[table[i].flight_no-1];
						if (bookedseat >= table[i].no_of_seats)
						{
							args->shmptr[table[i].flight_no-1] += table[i].no_of_seats;
							printf("\nCanceled :: [%d] seats canceled from flight no: [%d] by thread: %d\n",table[i].no_of_seats,table[i].flight_no,table[i].thread_no);
						}
						else
							printf("\nProhibited :: [%d] trying to cancel more than booked from flight [%d]\n", table[i].thread_no,table[i].flight_no);
					}
					else
						printf("\nThread [%d] didn't book any seat on flight [%d].\n",table[i].thread_no,table[i].flight_no);
					pthread_mutex_unlock(&write_lock[table[i].flight_no-1]); // if no write lock you unlock since you locked
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
	* Initialize mutexes
	*/
	for (i = 0; i < NOOFFLIGHTS; ++i)
	{
		pthread_mutex_init(&read_lock[i], NULL);
		pthread_mutex_init(&write_lock[i], NULL);
	}

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
	// Initialize flights with its capacity
	for (i = 0; i < NOOFFLIGHTS; ++i){
		shmptr[i] = 150;
	}
	printf("Server has attached the shared memory...\n");

	// Creat a new child process
	printf("Server is about to fork a child process...\n");
	pid = fork();
	if (pid < 0) {
	  printf("*** fork error (server) ***\n");
	  exit(1);
	}
	else if (pid == 0) {

		time_t start = time(NULL);
		time_t end = time(NULL);
		time_t server_life = 1;

		for (i = 0; i < MAX; ++i){
			sem_init(&sem[i],0,1);
		}

		// Open given file
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
			arg[i]->threadId = i;
			for (j = 0; j < NOOFFLIGHTS; ++j)
			{
				arg[i]->bookedStatus[j] = 0;
			}
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
			for (i = 0; i < MAX; ++i)
			{
				if (!table[i].valid)
				{
					sem_wait(&sem[i]);

					if(fscanf(fp,"%d %s %d %d",&qtime,qtype,&flight_no,&thread_no) != EOF){
						if(strcmp(qtype,"Inquiry"))
							fscanf(fp,"%d",&no_of_seats);
						table[i].valid = 1;
						table[i].query_time = qtime;
						strcpy(table[i].query_type,qtype);
						table[i].flight_no = flight_no;
						table[i].thread_no = thread_no;
						table[i].no_of_seats = no_of_seats;
					}
					else{
						//flag = 0;
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

			// printf("diff: %ld\n", end - start);
			// if(server_life <= (end - start)){
			// 	flag = 0;
			// }
			// end = time(NULL);
			// if (count >= 10)
			// {
			// 	flag = 0;
			// }
		}
		//printf("count :%d\n", count);
		/*
		* Wait till threads are complete before main child thread continues.
    	* Unless we wait if we run  exit which will terminate 
    	* the process and all threads before the threads completes execution. 
    	*/

    	for (i = 0; i < NOOFTHREADS; ++i)
    	{
    		pthread_mutex_destroy(&read_lock[i]);
	       	pthread_mutex_destroy(&write_lock[i]);
    		pthread_join(users[i],NULL);
    	}
    	
		exit(0); // return to parent process
	}

	wait(NULL);
	printf("Server has detected the completion of its child...\n");
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