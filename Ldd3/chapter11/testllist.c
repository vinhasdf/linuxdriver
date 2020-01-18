#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void main()
{
	int i;
	int fd;
	char buff[100];
	fd = open("/dev/llist", O_RDWR);
	if(fd < 0)
	{
		printf("Cannot open file\n");
		return;
	}

	srand(time(0));
	memset(buff, 0, 100);

	strcpy(buff, " Data");

	for(i = 0; i < 10; i++)
	{
		buff[0] = (rand()%10) + 0x30;
		write(fd, buff, 100);
	}

	for(i = 0; i < 10; i++)
	{
		memset(buff, 0, 100);
		if(read(fd, buff, 100) > 0)
		{
			printf("%s\n", buff);
		}
	}

	close(fd);
}