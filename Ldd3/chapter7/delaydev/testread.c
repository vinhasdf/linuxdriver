#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
//#include <pthread.h>

int fd, ret;

//void *thread1(void *arg)
//{
//	sleep(3);
//	printf("Release main thread\n");
//	write(fd, "Hello", 5);
//}

void main(void)
{
	//pthread_t mthread;
	char buff[100];
	fd = open("/dev/delay_dev0", O_RDWR);
	if(fd < 0)
	{
		printf("Error to open\n");
		return;
	}
	//pthread_create(&mthread, NULL, thread1, NULL);

	memset(buff, 0, 100);
	//strcpy(buff, "Hello lol");
	printf("Block for read\n");
	ret = read(fd, buff, 30);
	printf("%s with %d char\n", buff, ret);
	//pthread_join(mthread, NULL);
}
