#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
	char *arg[20],path[40];
	int i;
	for (i = 0; i < 10; ++i)
	{
		snprintf(path,40,"./output%d",i);
		if (fork() == 0)
		{
			if(execvp(path,arg) < 0)
				printf("Error!!\n");
			exit(0);
		}
		else
			wait(NULL);
	}
	for (i = 0; i < 2; ++i)
	{
		snprintf(path,40,"./plot%d",i);
		if (fork() == 0)
		{
			if(execvp(path,arg) < 0)
				printf("Error!!\n");
			exit(0);
		}
		else
			wait(NULL);
	}

	return 0;
}