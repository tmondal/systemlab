#include <stdio.h>

int main(int argc, char const *argv[])
{
	
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