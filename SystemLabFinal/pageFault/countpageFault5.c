#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>

#define SIZE 10240

int main(int argc, char const *argv[])
{
	char dummy,path[100];
	int i,size,fd;   
	unsigned char *addr;
    struct stat sb;
	struct rusage usage;
	
	snprintf(path, 100, "dd if=/dev/zero of=test1.txt bs=%dKB count=1", SIZE);
	system(path);
	
	FILE *output = fopen("output.txt","a");

	//To free pagecache, dentries and inodes:
	printf("Cache removing...\n");
	system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches && swapoff -a && swapon -a'");
	printf("Cache removed.\n");


    const char * file_name = "test1.txt";
    fd = open (file_name, O_RDONLY);
    if (fd < 0)
    {
    	perror("open");
    }

    // Get the size of the file from stat structure
    if (fstat(fd, &sb) < 0)
        perror("fstat");
    size = sb.st_size;

    addr = (char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED)
        perror("mmap");
    for (i = 0; i < size; i++) {
		dummy = addr[i];
    }

	getrusage(RUSAGE_SELF,&usage); // RUSAGE_SELF used to get resource usage of calling process

	printf("No of major pagefault: %ld\n", usage.ru_majflt);
	printf("No of minor pagefault: %ld\n", usage.ru_minflt);
	fprintf(output, "%d %ld %ld\n", SIZE, usage.ru_majflt,usage.ru_minflt);
	fclose(output);
	return 0;
}