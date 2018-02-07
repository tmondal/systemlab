#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>

struct read_format {
  uint64_t nr;
  struct {
    uint64_t value;
    uint64_t id;
  } values[];
};

void do_something() {
  char ch;
  system("dd if=/dev/zero of=test.txt bs=100MB count=1");
  //To free pagecache, dentries and inodes:
  printf("Cache removing...\n");
  system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches && swapoff -a && swapon -a'");
  printf("Cache removed.\n");
  FILE *fp = fopen("test.txt","r");
  while((ch = fgetc(fp)) != EOF){}
}

static long perf_event_open(struct perf_event_attr *hw_event,pid_t pid,int cpu,int group_fd,unsigned long flags) {
  int ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
  return ret; // returns file descriptor
}

int main(int argc, char* argv[]) {
  struct perf_event_attr pea;
  int fd1, fd2;
  uint64_t id1, id2;
  uint64_t val1, val2;
  

  memset(&pea, 0, sizeof(struct perf_event_attr));
  pea.type = PERF_TYPE_SOFTWARE;
  pea.size = sizeof(struct perf_event_attr);
  pea.config = PERF_COUNT_SW_PAGE_FAULTS_MIN;
  pea.disabled = 1;
  pea.exclude_kernel = 1;
  pea.exclude_hv = 1;
  pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
  fd1 = perf_event_open(&pea, 0, -1, -1, 0);

  //retrieve identifier for the first counter
  ioctl(fd1, PERF_EVENT_IOC_ID, &id1);

  memset(&pea, 0, sizeof(struct perf_event_attr));
  pea.type = PERF_TYPE_SOFTWARE;
  pea.size = sizeof(struct perf_event_attr);
  pea.config = PERF_COUNT_SW_PAGE_FAULTS_MAJ;
  pea.disabled = 0;
  pea.exclude_kernel = 1;
  pea.exclude_hv = 1;
  pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
  fd2 = perf_event_open(&pea, 0, -1, fd1 /*must be leader fd*/, 0);
  ioctl(fd2, PERF_EVENT_IOC_ID, &id2);


  ioctl(fd1, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
  ioctl(fd1, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
  do_something();
  ioctl(fd1, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);


  // Now read both counters parallely which is possible because of read_format = PERF_FORMAT_GROUP
  char buf[4096];
  struct read_format* rf = (struct read_format*) buf;
  int i;

  read(fd1, buf, sizeof(buf));
  for (i = 0; i < rf->nr; i++) {
    if (rf->values[i].id == id1) {
      val1 = rf->values[i].value;
    } else if (rf->values[i].id == id2) {
      val2 = rf->values[i].value;
    }
  }

  printf("No of minor page faults: %"PRIu64"\n", val1);
  printf("No of major page faults: %"PRIu64"\n", val2);   

  return 0;
}