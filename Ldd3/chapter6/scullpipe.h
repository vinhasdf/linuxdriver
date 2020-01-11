#ifndef SCULLPIPE_H
#define SCULLPIPE_H

#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>

#define SCP_NUM_DEVICES 2
#define SCP_DEFAULT_BUFFER_SIZE 30

struct scull_pipe{
	wait_queue_head_t rqu, wqu; // hold queue list header of process read and write
	char *beginbuff; // start position of buffer 
	int rp;
	char wp;
	int buffer; // buffer size
	struct semaphore sem;
	struct cdev cdev;
};

#endif