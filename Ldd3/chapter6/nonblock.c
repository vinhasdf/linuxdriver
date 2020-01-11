#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[])
{
	char buffer[1024];
	int delay = 1, r_err, w_err;
	if(argc > 1)
		delay = atoi(argv[1]);
	fcntl(0, F_SETFL, fcntl(0, F_GETFL)|O_NONBLOCK); // stdin
	fcntl(1, F_SETFL, fcntl(1, F_GETFL)|O_NONBLOCK); // stdout
	while(1)
	{
		r_err = read(0, buffer, 500);
		if(r_err > 0)
		{
			w_err = write(1, buffer, r_err);
		}
		if((r_err < 0 || w_err < 0) && (errno != EAGAIN))
			break;
		sleep(delay);
		//printf("lol\n");
	}
	perror(r_err < 0 ? "stdin" : "stdout");
	return 1;
}