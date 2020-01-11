#ifndef SCULL_H
#define SCULL_H

#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/ioctl.h>

#define SCULL_MAJOR 0
#define SCULL_NR_DEVS 4
#define QUANTUM_SIZE 10
#define QUANTUMSET_SIZE 2

struct scull_qset{
	char **data;
	struct scull_qset *next;
};

struct scull_dev {
	struct scull_qset *data;
	int quantum;
	int qset;
	unsigned long size;
	struct semaphore sem;
	struct cdev cdev;
};

/* ioctl cmd section */

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

/* end of ioctl cmd section */

#endif