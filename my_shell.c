#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

#define COMMANDLEN 100
#define MAXARGS 10
#define NOOFBUILTIN 3
// Clearing the shell using escape sequences
#define clear() printf("\033[H\033[J")



void welcomeMessage(){
	clear();
	printf("********************************************************************************\n");
	printf("*\n");
	printf("*\n");
	printf("*\t\t\tWelcome to the custom Shell\n");
	printf("*\n");
	printf("*\n");			
	printf("********************************************************************************\n");
	sleep(2);
	clear();
}

void printCurrentDirectory(){

	char path[1024];
	getcwd(path,sizeof(path));
	printf("%s", path);
	printf("$ ");
}

int getUserCommand(char *command){
	char *lineptr;
	lineptr = readline("");
	if (strlen(lineptr) != 0){
		//printf("len: %d\n", strlen(lineptr));
		strcpy(command,lineptr);
		return 1;
	}
	else 
		return 0;
}

int parseGreater(char *command){
	char *args[2];
	int i;
	for (i = 0; i < 2; ++i)
	{
		args[i] = strsep(&command,">");
		printf("%s\n", args[i]);
		if (args[i] == NULL)
			break;
	}
	if(args[1] == NULL){
		return 0; // neither > nor >> exists
	}
	else{
		// handle > here
		pid_t pid;
		pid = fork();
		if(pid < 0){
			printf("Error forking !!\n");
			return 1;
		}
		if (pid == 0)
		{
			// First open a file corresponding to args[1]
			int fd = open(args[1],O_CREAT | O_WRONLY);
			if(fd < 0){
				printf("Couldn't open the file.\n");
				return 1;  // since > exists
			}
			printf("here\n");
			if(dup2(fd,1) < 0) // STDOUT and fd points to same file now
				printf("Error duplicating!!\n");
			//close(fd); // close unnesessary fd
			printf("fd: %d\n", fd);
			// Now execute cat command 
			if (execvp(args[0],args) < 0)
			{
				printf("Couldn't execute cat command.\n");
				exit(0);
			}
		}
		else{
			wait(NULL); // wait untill child die
		}

		return 1;
	}
}

int parseGreaterGreater(char *command){
	char *args[2];
	int i;
	for (i = 0; i < 2; ++i)
	{
		args[i] = strsep(&command,">>");
		if (args[i] == NULL)
			break;
	}
	if(args[1] == NULL)
		return 0;
	else{

		// process >> here
		pid_t pid;
		pid = fork();
		if(pid < 0){
			printf("Error forking !!\n");
			return 1;
		}
		if (pid == 0)
		{
			// First open a file corresponding to args[1]
			int fd = open(args[1],O_APPEND);
			printf("fd: %d\n", fd);
			dup2(fd,1); // STDOUT and fd points to same file now
			close(fd); // close unnesessary fd
			// Now execute cat command 
			if (execvp(args[0],args) < 0)
			{
				printf("Couldn't execute cat command.\n");
				exit(0);
			}
		}
		else{
			wait(NULL); // wait untill child die
		}

		return 1;
	}
}

int parseCommand(char *command, char **args){


	//printf("len: %s\n", strlen(command));
	// int flag = parseGreater(command);
	//printf("done\n");
	
	int i;
	// String Parse logic
	for (i = 0; i < MAXARGS; i++){
		// Tokenize the string separated by " "
		args[i] = strsep(&command," ");
		// Check if more words
		if (args[i] == NULL)
			break;
		// Check if word is of 0 length
		if (strlen(args[i]) == 0)
			i--; // don't consider the word
	}
}

int callBuiltIn(char **args){

	/*
	* Define builtin commands
	* Compare the first word 
	* If a match do proper action
	* Else return 0 
	*/
	int i,index = 5;
	char *builtin[NOOFBUILTIN];
	builtin[0] = "cd";
	builtin[1] = "exit";
	builtin[2] = "help";

	for (i = 0; i < NOOFBUILTIN; i++){
		if (strcmp(args[0], builtin[i]) == 0 ){
			index = i + 1;
			break;
		}
	}

	switch(index) {

		case 1: 
			if (args[1] == NULL)
				chdir("/home");
			else
				chdir(args[1]);
			return 1;
		case 2:
			printf("\nYou have exited from custom Shell :)\n\n");
			exit(0);
		case 3:
			printf("\n\n**********************************************************************\n");
			printf("*\n");
			printf("*\tType command and arguments with space separated, and hit enter.\n");
			printf("*\tThe following are built in commands:\n");
			for (i = 0; i < NOOFBUILTIN; i++) {
				printf("*\t\t[%d]  %s\n", i, builtin[i]);
			}
			printf("*\tType \"man <command_name>\" for information of other commands.\n\n");
			printf("**********************************************************************\n\n");
			return 1;
		default:
			break;
	}

	return 0;
}

void callExecvp(char **args){

	// Creating a child
    pid_t pid = fork(); 
 
    if (pid == -1) {
        return;
    }else if (pid == 0) {
        if (execvp(args[0], args) < 0) {
            printf("%s : not a valid Linx command :(\n",args[0]);
        }
        exit(0);
    }else {
        // waiting for child to terminate
        wait(NULL); 
        return;
    }
}


int main(int argc, char const *argv[]){

	char commandString[COMMANDLEN], *parsedArgs[MAXARGS];
	int flag;
	// Welcome message
	welcomeMessage();

	// Get current username 
	char *username = getenv("USER");

	// Main loop for executing commands
	while(1){

		/* 	
		* First print username@username:
		* Print the path to current dir
		* Append $ at the end
		*/
		printf("\n%s@%s:~", username,username);
		printCurrentDirectory();

		// Read the input line from standard input
		if(!getUserCommand(commandString)){
			continue;
		}

		// Parse the entered string to get command and arguments
		parseCommand(commandString,parsedArgs);
		// if(!v)
		// 	continue;
		/*
		* Execute the parsed command 
		* First call builtin
		* If not builtin call execvp
		*/
		flag = callBuiltIn(parsedArgs);
		if (!flag){
			callExecvp(parsedArgs);
		}
		//char ch = getchar();
	}
	return 0;
}