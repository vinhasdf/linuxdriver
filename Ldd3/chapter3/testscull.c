#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define SCULL_IOC_MAGIC 'k'

#define SCULL_IOCRESET _IO(SCULL_IOC_MAGIC, 0)
/* 
 * S: Set from pointer
 * T: Tell directly with argument value
 * G: Get from pointer
 * Q: Query from return value
 * X: exchange, switch G and S automaticly
 * H: shift, switch T and Q automaticly
 */
#define SCULL_IOCSQUANTUM 	_IOW(SCULL_IOC_MAGIC, 1, int)
#define SCULL_IOCSQSET		_IOW(SCULL_IOC_MAGIC, 2, int)
#define SCULL_IOCTQUANTUM	_IO(SCULL_IOC_MAGIC, 3)
#define SCULL_IOCTQSET		_IO(SCULL_IOC_MAGIC, 4)
#define SCULL_IOCGQUANTUM 	_IOR(SCULL_IOC_MAGIC, 5, int)
#define SCULL_IOCGQSET		_IOR(SCULL_IOC_MAGIC, 6, int)
#define SCULL_IOCQQUANTUM	_IO(SCULL_IOC_MAGIC, 7)
#define SCULL_IOCQQSET		_IO(SCULL_IOC_MAGIC, 8)
#define SCULL_IOCXQUANTUM 	_IOWR(SCULL_IOC_MAGIC, 9, int)
#define SCULL_IOCXQSET		_IOWR(SCULL_IOC_MAGIC, 10, int)
#define SCULL_IOCHQUANTUM	_IO(SCULL_IOC_MAGIC, 11)
#define SCULL_IOCHQSET		_IO(SCULL_IOC_MAGIC, 12)

void main()
{
	int quantum, qset;
	int retval;
	int fd;
	fd = open("/dev/scull0", O_RDWR);
	if(fd < 0)
	{
		printf("Cannot open file\n");
	}

	if(ioctl(fd, SCULL_IOCGQUANTUM, &quantum))
	{
		printf("Error to get quantum\n");
	}
	else
	{
		printf("Quantum = %d\n", quantum);
	}

	if(ioctl(fd, SCULL_IOCGQSET, &qset))
	{
		printf("Error to get qset\n");
	}
	else
	{
		printf("Qset = %d\n", qset);
	}

	if(ioctl(fd, SCULL_IOCTQUANTUM, 11))
	{
		printf("Cannot set quantum\n");
	}
	else
	{
		retval = ioctl(fd, SCULL_IOCQQUANTUM);
		printf("Quantum = %d\n", retval);
	}

	if(ioctl(fd, SCULL_IOCTQSET, 3))
	{
		printf("Cannot set qset\n");
	}
	else
	{
		retval = ioctl(fd, SCULL_IOCQQSET);
		printf("Qset = %d\n", retval);
	}
}