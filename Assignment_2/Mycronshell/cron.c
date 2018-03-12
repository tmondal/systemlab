#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>


void handle_sigchild(int sig)
{
    int status;
    waitpid(-1, &status, 0); // same as wait()
    signal(SIGCHLD,handle_sigchild);
}

void daemonize()
{

	char directory[20] = "/home/";
    pid_t pid = fork();

    if (pid == -1) perror("fork error");
    if (pid != 0)exit(0); // parent exits
    /*
	* Make sure all the fd's are disconected from daemon
    */
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
    /*
    * Session makes sure a process has no link with terminal
    * create session in child since child will have a pid which is not 
    * group leader pid
    */
    setsid();

    /* 
    * Making sure child is not session leader either
    */
    pid = fork();

    if (pid == -1) perror("double fork error");
    if (pid != 0) exit(0); 

    /*
	* change directory to "/home/username/" because if daemon creates any file in mounted file system
	* the file system cann't be unmounted because daemon will run in the background
    */
    strcat(directory,getenv("USER"));
    chdir(directory);
}

int matchWday(int wday, char *time){
	

	if (isalpha(*time)){
		if (!strcmp(time,"sun") || !strcmp(time,"Sun") || !strcmp(time,"SUN") && wday == 0)
			return 1;
		if (!strcmp(time,"mon") || !strcmp(time,"Mon") || !strcmp(time,"MON") && wday == 1)
			return 1;
		if (!strcmp(time,"tue") || !strcmp(time,"Tue") || !strcmp(time,"TUE") && wday == 2)
			return 1;
		if (!strcmp(time,"wed") || !strcmp(time,"Wed") || !strcmp(time,"WED") && wday == 3)
			return 1;
		if (!strcmp(time,"thu") || !strcmp(time,"Thu") || !strcmp(time,"THU") && wday == 4)
			return 1;
		if (!strcmp(time,"fri") || !strcmp(time,"Fri") || !strcmp(time,"FRI") && wday == 5)
			return 1;
		if (!strcmp(time,"sat") || !strcmp(time,"Sat") || !strcmp(time,"SAT") && wday == 6)
			return 1;
	}
}

int matchMonth(int sys_time, char *time){
	if (isalpha(*time)){

		if ((!strcmp(time,"jan") || !strcmp(time,"Jan") || !strcmp(time,"JAN")) && sys_time == 0)
			return 1;
		if (!strcmp(time,"feb") || !strcmp(time,"Feb") || !strcmp(time,"FEB") && sys_time == 1)
			return 1;
		if (!strcmp(time,"mar") || !strcmp(time,"Mar") || !strcmp(time,"MAR") && sys_time == 2)
			return 1;
		if (!strcmp(time,"apr") || !strcmp(time,"Apr") || !strcmp(time,"APR") && sys_time == 3)
			return 1;
		if (!strcmp(time,"may") || !strcmp(time,"May") || !strcmp(time,"MAY") && sys_time == 4)
			return 1;
		if (!strcmp(time,"jun") || !strcmp(time,"Jun") || !strcmp(time,"JUN") && sys_time == 5)
			return 1;
		if (!strcmp(time,"jul") || !strcmp(time,"Jul") || !strcmp(time,"JUL") && sys_time == 6)
			return 1;
		if (!strcmp(time,"aug") || !strcmp(time,"Aug") || !strcmp(time,"AUG") && sys_time == 7)
			return 1;
		if (!strcmp(time,"sept") || !strcmp(time,"Sept") || !strcmp(time,"SEPT") && sys_time == 8)
			return 1;
		if (!strcmp(time,"oct") || !strcmp(time,"Oct") || !strcmp(time,"OCT") && sys_time == 9)
			return 1;
		if (!strcmp(time,"nov") || !strcmp(time,"Nov") || !strcmp(time,"NOV") && sys_time == 10)
			return 1;
		if (!strcmp(time,"dec") || !strcmp(time,"Dec") || !strcmp(time,"DEC") && sys_time == 11)
			return 1;
		return 0;
	}

}

int match(int sys_time, char *time, int llimit, int ulimit, int flag){
	
	int i,val;
	char *min, *max;
	int posval;
	int left,right;

	/*
	* If line starts with # that implies a comment . So cron ignore that line
	*/
	if (*time == '#')
		return 0;
	/*
	* If the time field starts with alphabet , it must be either Month or Day name
	* So handle accordingly
	*/
	if (isalpha(*time)){
		if (flag == 1)
			return matchMonth(sys_time,time);
		if (flag == 2)
			return matchWday(sys_time,time);
	}

	/*
	* If line time field is * that implies always match so return 1
	*/
	if (*time == '*')
		return 1;
	else{
		if (isdigit(*time))
		{
			/*
			* Seperate string if any of the [/ - ,] apears in the string else simply use atoi
			* to get corresponding interger value and match with system time (sent via argument)
			*/
			if (strchr(time,'/'))
			{
				min = strsep(&time,"/");
				max = strsep(&time,"/");

				left = atoi(min);
				right = atoi(max);

				if (right > 0)
				{
					posval = left;
					while(posval <= ulimit){
						if (posval == sys_time)
							return 1;
						posval += right;
					}
					return 0;
				}
				return 0;
			}
			else if (strchr(time,'-'))
			{
				min = strsep(&time,"-");
				max = strsep(&time,"-");

				left = atoi(min);
				right = atoi(max);

				if (left <= right)
				{
					for (i = left; i <= right; ++i)
					{
						if (i == sys_time)
							return 1;
					}
				}
				else
					return 0;
			}
			else if (strchr(time,','))
			{
				while((min = strsep(&time,",")) != NULL ){
					left = atoi(min);
					if (left == sys_time)
						return 1;
				}
				return 0;
			}
			else{
				left = atoi(time);
				if (left == sys_time)
					return 1;
				return 0;
			}
		}
		
		return 0;
	}
}

void runcommand(char *command){

	strcat(command,">> mycron.log");
	system(command);
}

void timeMatched(struct tm *tm){

	/*
	* Read a line and match time 
	* if match run corresponding command using system function
	* else goto next line 
	*/
	char err[100] = "echo \"could not find mycrontab.txt\" >> mycron.log";
	FILE *fp = fopen("mycrontab.txt","r");
	if(fp == NULL){
		system(err);
		exit(1);
	}
	
	char minute[60],hour[60],dom[60],month[60],dow[60],command[100];
	while(fscanf(fp,"%s %s %s %s %s %[^\n]\n",minute,hour,dom,month,dow,command) != EOF){

		if (match(tm->tm_min,minute,0,59,0) && match(tm->tm_hour,hour,0,23,0) && match(tm->tm_mday,dom,1,31,0) && match(tm->tm_mon,month,0,11,1) 
			&& match(tm->tm_wday,dow,0,6,2))
		{
			runcommand(command);
		}
	}
	fclose(fp);

}


int main(int argc, char const *argv[])
{

	struct tm *tm;
	time_t now;

	daemonize();

	/*
	* Handle any signal related to terminal
	*/
	signal(SIGCHLD, handle_sigchild);
	signal(SIGHUP, SIG_IGN);
	int i = 5;
	while( i >= 0){

		time(&now);
		tm = localtime(&now);
		/*
		* compare current time and crontab entry if match call runjob function
		*/
		timeMatched(tm);
		sleep(60);
		i--;
	}

	return 0;
}