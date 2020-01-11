#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

void main(void)
{
	int fd, ret;
	char buff[100];
	fd = open("/dev/scullpipe0", O_RDWR);
	if(fd < 0)
	{
		printf("Error to open\n");
		return;
	}
	memset(buff, 0, 100);
	strcpy(buff, "Hello lol");
	printf("Block for write\n");
	ret = write(fd, buff, strlen(buff));
	printf("Write with %d char\n", ret);
}