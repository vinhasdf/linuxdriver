#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

void main()
{
	int i;
	int fd;
	char buff[100];
	fd = open("/dev/blink", O_RDWR);
	if(fd < 0)
	{
		printf("Cannot open file\n");
		return;
	}
	for(i = 0; i < 10; i++)
	{
		memset(buff, 0, 100);
		buff[0] = i;
		write(fd, buff, 1);
		if(read(fd, buff, 10) > 0)
		{
			printf("%d\n", buff[0]);
		}
		sleep(1);
	}
	close(fd);
}