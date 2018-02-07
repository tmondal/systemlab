// C Program to design a shell in Linux
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
 
#define MAXCOMLENGTH 100 // max number of letters to be supported
#define MAXNOOFCOMMAND 10 // max number of commands to be supported
#define NOOFBUILTIN 3 // number of builtin command
 
// Clearing the shell using escape sequences
#define clear() printf("\033[H\033[J")
 
// Greeting shell during startup
void welcomeMessage()
{
    clear();
    printf("********************************************************************************\n");
    printf("*\n");
    printf("*\n");
    printf("*\t\t\tWelcome to the custom Shell\n");
    printf("*\n");
    printf("*\n");          
    printf("********************************************************************************\n");
    sleep(1);
    clear();
}
 
// Function to take input
int getUserCommand(char* str)
{
    char* buf;
    buf = readline(" ");
    if (strlen(buf) != 0) {
        add_history(buf);
        strcpy(str, buf);
        return 0;
    } else {
        return 1;
    }
}

// Count arguments
int countArgs(char *command){
    int i = 0,n = 0;
    while(i < strlen(command)){
        if(command[i] == ' ')
            n++;
        if(n == 12){
            printf("Usage: maximum 10 arguments allowed.\n");
            return 0;
        }
        i ++;
    }

    return 1;
}
 
// Function to print Current Directory.
void printCurrentDirectory()
{
    char *username = getenv("USER");
    printf("%s@%s:~", username,username);
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s", cwd);
}
 
// Function where the system command is executed
void esecuteExecvp(char** parsed)
{
    // Forking a child
    pid_t pid = fork(); 
 
    if (pid == -1) {
        printf("\nFailed forking child..");
        return;
    } else if (pid == 0) {
        if (execvp(parsed[0], parsed) < 0) {
            printf("Could not execute command.\n");
        }
        exit(0);
    } else {
        // waiting for child to terminate
        wait(NULL); 
        return;
    }
}
 
// Function where the piped system commands is executed
void executeRedirectioncommand(char** parsed, char** parsedpipe)
{
    
    pid_t p1;

    p1 = fork();
    if (p1 < 0) {
        printf("\nCould not fork");
        return;
    }
 
    if (p1 == 0) {
        
        int fd = open(parsedpipe[0],O_WRONLY | O_CREAT,0644);
        if(fd < 0){
            printf("Couldn't open the file.\n");
            return;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
 
        if (execvp(parsed[0], parsed) < 0) {
            exit(0);
        }
    } 
    else {
        wait(NULL);
    }
}
 
// Function to execute builtin commands

int callBuiltIn(char **args){

    /*
    * Define builtin commands
    * Compare the first word 
    * If a match take proper action and return 1
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
                chdir("/");
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
 
// function for finding redirection operator(>)
int parseGreater(char* str, char** leftStringRightString)
{
    /*
    * check if > is present 
    * if present get two string left of > and right of > and return 1
    * else return 0 indication no > found
    */
    int i;
    for (i = 0; i < 2; i++) {
        leftStringRightString[i] = strsep(&str, ">");
        if (leftStringRightString[i] == NULL)
            break;
    }
 
    if (leftStringRightString[1] == NULL)
        return 0;
    else {
        return 1;
    }
}
 
// function for parsing command into set of words
void parseSpace(char* str, char** wordsofCommand)
{
    int i;
 
    for (i = 0; i < MAXNOOFCOMMAND; i++) {
        wordsofCommand[i] = strsep(&str, " ");
 
        if (wordsofCommand[i] == NULL)
            break;
        if (strlen(wordsofCommand[i]) == 0)
            i--;
    }
}
 
int processString(char* command, char** leftString, char** rightString)
{
    /*
    * returns zero if there is no command or it is a builtin command,
    * 1 if it is a simple command i.e no redirection exists and not builtin 
    * 2 if it is including a redirection operator.
    */

    char* leftandRightstring[2];
    int greater = 0;
 
    greater = parseGreater(command, leftandRightstring);
 
    if (greater) {
        parseSpace(leftandRightstring[0], leftString); 
        parseSpace(leftandRightstring[1], rightString); 
    } else { 
        parseSpace(command, leftString);
    }
 
    if (callBuiltIn(leftString))
        return 0;
    else
        return 1 + greater;
}
 
int main()
{
    char commandString[MAXCOMLENGTH];
    char *leftString[MAXNOOFCOMMAND], * rightString[MAXNOOFCOMMAND];
    int flag = 0;
    welcomeMessage();
 
    while (1) {
        // print current directory at the begining and after a command executed
        printCurrentDirectory();
        // get user command
        if (getUserCommand(commandString))
            continue;

        // Count how many arguments entered
        if(!countArgs(commandString))
            continue;
        // Process the given command 
        flag = processString(commandString,leftString, rightString);
       
        if (flag == 1)
            esecuteExecvp(leftString);
 
        if (flag == 2)
            executeRedirectioncommand(leftString, rightString);
    }
    return 0;
}