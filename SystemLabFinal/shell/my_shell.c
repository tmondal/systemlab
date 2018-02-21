// C Program to design a shell in Linux
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h> // Directory read,close etc
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>

 
#define MAXCOMLENGTH 100 // max number of letters to be supported
#define MAXNOOFCOMMAND 10 // max number of commands to be supported
#define NOOFBUILTIN 3 // number of builtin command
 
// Clearing the shell using escape sequences
#define clear() printf("\033[H\033[J")


void welcomeMessage()
{
    clear();
    printf("********************************************************************************\n");
    printf("*\n");
    printf("*\n");
    printf("*\t\t\tWelcome to the custom Shell\n");
    printf("*\n");
    printf("*\t\t\tType <help> for help.\n");
    printf("*\n");          
    printf("********************************************************************************\n");
    sleep(2);
    clear();
}
 
// Function to get inputs
int getUserCommand(char* str,char *path)
{
    
    // printf("%s ", temp);


    char* buf;
    buf = readline(path);
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
 
// Function to print current directory.
char *printCurrentDirectory()
{
    // char *username = getenv("USER");
    // printf("%s@%s:~", username,username);
    // char cwd[1024];
    // getcwd(cwd, sizeof(cwd));
    // printf("%s$ ", cwd);
    char *temp = (char*)malloc(sizeof(char)*1024);
    char *username = getenv("USER");
    strcpy(temp,username);
    strcat(temp,"@");
    strcat(temp,username);
    strcat(temp,":~");
    // printf("%s@%s:~", username,username);
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    strcat(temp,cwd);
    strcat(temp,"$");
    //printf("%s", temp);
    return temp;
}

void print_process_status(long int process_no) {
    char path[40], line[100], *p,property[20];
    FILE* status_fp;
    int val,pid;
    char size[10];
    // Since fopen takes const path(no variable allowed in the path) we first create path with snprintf
    snprintf(path, 40, "/proc/%ld/status", process_no);

    status_fp = fopen(path, "r");
    if(!status_fp)
        return;

    while(fgets(line, 100, status_fp)) {

        /*
        * int strncmp(const char *s1, const char *s2, size_t n);
        * strncmp compares first n bytes of s1 and s2 if equal return 0
        */
        if(strncmp(line, "Pid:", 4) == 0){
            // Ignoring "Pid:" and whitespace
            p = line + 5;
            while(isspace(*p)) ++p;
            // printf prints strating from p till \0 found
            printf("\t");
            while(*p != '\n')
                printf("%c", *(p++));
            printf("\t\t");
        }
        else if(strncmp(line, "VmSize:", 7) == 0){
            p = line + 8;
            while(isspace(*p)) ++p;
            while(*p != '\n')
                printf("%c", *(p++));
            printf("\t\t");
        } 
        else if(strncmp(line, "VmRSS:", 6) == 0){
            p = line + 7;
            while(isspace(*p)) ++p;
            while(*p != '\n')
                printf("%c", *(p++));
            printf("\t\t");
        }
    }
    fseek(status_fp,0,SEEK_SET);
    while(fgets(line, 100, status_fp)){
        if(strncmp(line, "State:", 6) == 0){
            p = line + 7;
            while(isspace(*p)) ++p;
            while(*p != '\n')
                printf("%c", *(p++));
            printf("\t");
        }
    }
    fseek(status_fp,0,SEEK_SET);
    while(fgets(line, 100, status_fp)){
        if(strncmp(line, "Name:", 5) == 0){
            p = line + 6;
            while(isspace(*p)) ++p;
            while(*p != '\n')
                printf("%c", *(p++));
            printf("\n");
        }
    }

    fclose(status_fp);
}
 
// Function where the system command is executed
void esecuteExecvp(char** leftString)
{
    // Forking a child
    pid_t pid = fork(); 
 
    if (pid == -1) {
        printf("\nFailed forking child..");
        return;
    } else if (pid == 0) {

        /*
        * Simple cat command and mkdir logic
        */ 

        int i;
        FILE *fp[MAXNOOFCOMMAND];
        int maxline = 1024;
        char *line = (char *)malloc(sizeof(char)*maxline);
        if (strcmp(leftString[0], "cat") == 0) {

            if (leftString[1] == NULL){
                while(fgets(line, maxline, stdin)){
                    printf("%s", line);
                }
                exit(0);
            }
            else{
                if (strcmp(leftString[1],"help") == 0 || strcmp(leftString[1],"-help") == 0 || strcmp(leftString[1],"--help") == 0)
                {
                    printf("Usage: cat <file name> ...<file name>\n");
                }
                else{
                    for (i = 1; i < MAXNOOFCOMMAND; ++i)
                    {
                        if(leftString[i] != NULL){
                            fp[i-1] = fopen(leftString[i],"r");
                            while(fgets(line, maxline, fp[i-1])){
                                printf("%s", line);
                            }
                            fclose(fp[i-1]);
                        }
                        else{
                            break;
                        }
                    }
                }
            }
            exit(0);
        }
        else if (strcmp(leftString[0],"mkdir") == 0)
        {
            if (leftString[1] == NULL)
            {
                printf("Usage : mkdir <folder name> \n");
            }
            else{
                if (strcmp(leftString[1],"help") == 0 || strcmp(leftString[1],"-help") == 0 || strcmp(leftString[1],"--help") == 0)
                {
                    printf("Usage: mkdir <folder name>\n");
                }
                else{
                    if (mkdir(leftString[1],0777) < 0){
                        int err = errno;
                        switch(err){
                            case EEXIST:
                                printf("Directory already exists.\n");
                                break;
                            case EACCES:
                                printf("Permission denied.\n");
                                break;
                            case EROFS :
                                printf("That's a file name.\n");
                                break;   
                        }
                    }
                }
            }
            exit(0);
        }

        else if (strcmp(leftString[0] , "top") == 0)
        {
            int val;
            int total,free,buff,used;
            char size[10];

            if (leftString[1] != NULL)
            {
                printf("Usage: top [No arguments]\n");
                exit(0);
            }

            /*
            * Print current memory usage
            */
            FILE *fp = fopen("/proc/meminfo","r");
            while(fscanf(fp,"%s%d%s",line,&val,size) != EOF){
                
                if (strcmp(line,"MemTotal:") == 0)
                {
                    total = val;
                    printf("%s Mem :    ",size);
                    printf("%d total    ", val);
                }
                else if (strcmp(line,"MemFree:") == 0)
                {
                    free = val;
                    printf("%d free    ", val);
                }
                else if (strcmp(line,"Cached:") == 0)
                {
                    buff = val;
                    used = total - free - buff;
                    printf("%d used    ", used);
                    printf("%d buff/cached\n", val);
                }
            }
            fseek(fp,0,SEEK_SET);
            while(fscanf(fp,"%s%d%s",line,&val,size) != EOF){
                if (strcmp(line,"SwapTotal:") == 0)
                {
                    printf("%s Swap :    ",size);
                    printf("%d total    ", val);
                    total = val;
                }
                else if (strcmp(line,"SwapFree:") == 0)
                {
                    free = val;
                    printf("%d free    ", val);
                }
            }
            printf("%d  used    ", total - free);
            fseek(fp,0,SEEK_SET);
            while(fscanf(fp,"%s%d",line,&val) != EOF){
                if (strcmp(line,"MemAvailable:") == 0)
                {
                    printf("%d  avail mem \n", val);
                }
            }
            fclose(fp);

            /*
            * Print current process information
            * 

                struct dirent {
                    ino_t          d_ino;       // inode number 
                    off_t          d_off;       // not an offset; see NOTES 
                    unsigned short d_reclen;    // length of this record 
                    unsigned char  d_type;      // type of file; not supported by all filesystem types 
                    char           d_name[256]; // filename 
                };

            */
            
            DIR* proc_dir = opendir("/proc");
            struct dirent* dir_entry;
            long int process_no;

            if(proc_dir == NULL) {
                perror("opendir(/proc)");
                return;
            }
            printf("\n\tPID\t\tVIRT\t\t\tRES\t\tSLEEP/RUNNING\tCOMMAND\n\n");
            while(dir_entry = readdir(proc_dir)) {

                // if directory name not a number don't consider
                if(!isdigit(*dir_entry->d_name))
                    continue;

                process_no = strtol(dir_entry->d_name, NULL, 10);

                // Read process status from status file of the corresponding process_no
                print_process_status(process_no);
            }
            // Finally close the /proc directory
            closedir(proc_dir);          
        }
        else{
            if (execvp(leftString[0],leftString) < 0)
            {
                printf("Can't execute the command :(\n");
            }
        }

        exit(0);
    } else {
        // Parent process waiting for child process to terminate
        wait(NULL); 
        return;
    }
}
 
// Function where the piped system commands is executed
void executeRedirectioncommand(char** leftString, char** rightString, int type)
{
    
    pid_t p1;
    int i;

    p1 = fork();
    if (p1 < 0) {
        printf("\nCould not fork");
        return;
    }
 
    if (p1 == 0) {

        int i,fd;
        if (leftString[2] != NULL)
        {
            printf("Usage: cat > <file name>\n");
            printf("Usage: cat >> <file name>\n");
            printf("Usage: cat <file name> [>] <file name>\n");
            printf("Usage: cat <file name> >> <file name>\n");
            return;
        }
        
        if (leftString[1] == NULL)
        {
            int fd;
            if (type == 2){
                fd = open(rightString[0],O_WRONLY | O_CREAT | O_APPEND ,0644);
                if(fd < 0){
                    printf("Couldn't open the file.\n");
                    return;
                }
            }
            if (type != 2)
            {
                fd = open(rightString[0],O_WRONLY | O_CREAT | O_TRUNC,0644);
                if(fd < 0){
                    printf("Couldn't open the file.\n");
                    return;
                }
            }

            int ch;
            dup2(fd, STDOUT_FILENO);
            while( (ch = fgetc(stdin)) != EOF ){
                printf("%c",ch);
            }
            close(1);
        }
        else if (leftString[1] != NULL)
        {
            FILE *fd;
            if (type == 2){
                fd = fopen(rightString[0],"a+");
                if(fd < 0){
                    printf("Couldn't open the file.\n");
                    return;
                }
            }
            if (type == 1)
            {
                fd = fopen(rightString[0],"w+");
                if(fd < 0){
                    printf("Couldn't open the file.\n");
                    return;
                }
            }
            FILE* in = fopen(leftString[1],"r");
            if(in < 0){
                printf("Couldn't open the file.\n");
                return;
            }

            int ch;
            while( (ch = fgetc(in)) != EOF ){
                fprintf(fd,"%c",ch);
            }
            fclose(in);
            fclose(fd);   
        }

        /*
        * Tried with fopen. Unsuccessfull
        */

        // if (leftString[2] != NULL)
        // {
        //     if (type == 2){
        //         FILE *fd = fopen(rightString[0],"a+");
        //         if(fd < 0){
        //             printf("Couldn't open the file.\n");
        //             return;
        //         }
        //     }
        //     FILE *fd = fopen(rightString[0],"w+");
        //     if(fd < 0){
        //         printf("Couldn't open the file.\n");
        //         return;
        //     }
        //     FILE* in = fopen(leftString[1],"r");
        //     FILE *in1 = fopen(leftString[2],"r");
        //     if(in < 0){
        //         printf("Couldn't open the file.\n");
        //         return;
        //     }

        //     int ch;
        //     // while( (ch = fgetc(in)) != EOF ){
        //     //     fprintf(fd,"%c",ch);
        //     // }
        //     while( (ch = fgetc(in1)) != EOF ){
        //         fprintf(fd,"%c",ch);
        //     }
        //     fclose(in1);
        //     fclose(in);
        //     fclose(fd); 

        //     return;
        // }

        /*
        * Tried with dup2 and open . Unsuccessfull
        */
        
        // if (type == 2)
        // {
        //     fd = open(rightString[0],O_WRONLY | O_CREAT | O_APPEND,0644);
        //     if(fd < 0){
        //         printf("Couldn't open the file.\n");
        //         return;
        //     }
        // }
        // if(type != 2){
        //     fd = open(rightString[0],O_WRONLY | O_CREAT | O_TRUNC,0644);
        //     if(fd < 0){
        //         printf("Couldn't open the file.\n");
        //         return;
        //     }
        // }
        // dup2(fd,STDOUT_FILENO);
        // close(fd);

        // FILE *fp[MAXNOOFCOMMAND];
        // int maxline = 1024;
        // char line[maxline];

        // if (leftString[1] == NULL)
        // {
        //     while(fgets(line, maxline, stdin)){
        //         printf("%s", line);
        //     }
        // }
        // else{
        //     for (i = 1; i < MAXNOOFCOMMAND; ++i)
        //     {
        //         printf("bal\n");
        //         if(leftString[i] != NULL){
        //             fp[i] = fopen(leftString[i],"r");
        //             while(fgets(line, maxline, fp[i])){
        //                 printf("%s", line);
        //             }
        //             fclose(fp[i]);
        //         }
        //         else{
        //             break;
        //         }
        //     }
        // }
        // close(1);
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
            if (args[1] == NULL){
                chdir("/");
                return 1;
            }
            else{
                if (strcmp(args[1],"help") == 0 || strcmp(args[1],"-help") == 0 || strcmp(args[1],"--help") == 0)
                {
                    printf("Usage: cd <folder name>\n");
                    return 1;
                }
                if(chdir(args[1]) < 0){
                    int err = errno;
                    switch(err){
                        case ENOENT:
                            printf("Directory doesn't exists.\n");
                            break;
                        case ENOTDIR:
                            printf("Path is not a directory.\n" );
                            break;
                        case EACCES:
                            printf("Access denied.\n");
                    }
                }
                return 1;
            }
        case 2:
            printf("\nYou have exited from custom Shell :)\n\n");
            exit(0);
        case 3:
            printf("\n\n**********************************************************************\n");
            printf("*\tValid commands for this shell are: \n");
            printf("*\t\t[1] cd\n");
            printf("*\t\t[2] mkdir\n");
            printf("*\t\t[3] cat with redirection\n");
            printf("*\t\t[4] top\n");
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
int parseGreater(char* str, char** leftStringRightString, int *type)
{
    /*
    * check if > is present
    * if not then return 0 
    * if present get two string left of > and right of >
    * if right of > is of o length then check for >>
    */
    char *store = (char *)malloc(sizeof(str));
    strcpy(store,str);
    char *temp[3];

    int i = 0,flag = 0;

    while(i < strlen(store)){
        if (store[i] == '>'){
            flag = 1;
            break;
        }
        i++;
    }
    if (!flag){
        return 0;
    }

    for (i = 0; i < 2; i++) {
        leftStringRightString[i] = strsep(&str, ">");
        if (leftStringRightString[i] == NULL)
            break;
    }
 
    if (strlen(leftStringRightString[1]) == 0){

        for (i = 0; i < 3; i++) {
            temp[i] = strsep(&store, ">");
            if (temp[i] == NULL)
                break;
        }
        leftStringRightString[0] = temp[0];
        leftStringRightString[1] = temp[2];
        *type = 2; // for >>
        return 1;
    }
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
 
int processString(char* command, char** leftString, char** rightString, int *type)
{
    /*
    * returns zero if there is no command or it is a builtin command,
    * 1 if it is a simple command i.e no redirection exists and not builtin 
    * 2 if it is including a redirection operator.
    */

    char* leftandRightstring[2];
    int greater = 0;
 
    greater = parseGreater(command, leftandRightstring, type);
 
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
    char commandString[MAXCOMLENGTH],*path;
    char *leftString[MAXNOOFCOMMAND]={NULL}, * rightString[MAXNOOFCOMMAND] = {NULL};
    int flag = 0;
    int type = 0; // to know > or >> or simple type
    welcomeMessage();
 
    while (1) {
        // print current directory at the begining and after a command executed
        path = printCurrentDirectory();
        // get user command
        if (getUserCommand(commandString,path))
            continue;

        // Count how many arguments entered
        if(!countArgs(commandString))
            continue;
        // Process the given command 
        flag = processString(commandString,leftString, rightString, &type);
       
        if (flag == 1)
            esecuteExecvp(leftString);
    
        if (flag == 2)
            executeRedirectioncommand(leftString, rightString,type);
    }
    
    return 0;
}