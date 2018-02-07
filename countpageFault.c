#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>



int main(int argc, char const *argv[])
{
	char ch;
	struct rusage usage;
	// char *command = "dd if=/dev/zero of=test.txt bs=5MB count=1";
	// FILE *fd = popen(command,"r");
	// pclose(fd);
	system("dd if=/dev/zero of=test.txt bs=1MB count=1");

	//FILE *fcache = popen("# sync; echo 3 > /proc/sys/vm/drop_caches","w");

	//To free pagecache, dentries and inodes:
	printf("Cache removing...\n");
	system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches && swapoff -a && swapon -a'");
	printf("Cache removed.\n");
	FILE *fp = fopen("test.txt","r");
	printf("Reading from created file...\n");

	while((ch = fgetc(fp)) != EOF){}
	printf("Reading done.\n"); 

	getrusage(RUSAGE_SELF,&usage); // RUSAGE_SELF used to get resource usage of calling process
	printf("No of major pagefault: %ld\n", usage.ru_majflt);
	printf("No of minor pagefault: %ld\n", usage.ru_minflt);
	return 0;
}