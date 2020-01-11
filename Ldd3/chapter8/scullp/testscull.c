#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

void main()
{
	int ret;
	int fd;
	char buff[100];
	fd = open("/dev/scullp0", O_RDWR);
	if(fd < 0)
	{
		printf("Cannot open file\n");
		return;
	}
	memset(buff, 0, 100);
	ret = write(fd, "hello 1", 7);
	ret = write(fd, ", hello 2", 9);
	close(fd);

	fd = open("/dev/scullp0", O_RDWR);
	if(fd < 0)
	{
		printf("Cannot open file\n");
		return;
	}
	ret = read(fd, buff, 30);
	if(ret > 0)
	{
		printf("Buff: %s with %d char\n", buff, ret);
	}
	else
	{
		printf("Khong co gi\n");
	}
	close(fd);
}