#include <stdio.h>
#include <stdlib.h>
#include "scull.h"

struct scull_qset *scull_follow(struct scull_dev *dev, int node)
{
	struct scull_qset *mdata;
	mdata = dev->data;
	if(mdata == NULL)
	{
		mdata = dev->data = (struct scull_qset *)malloc(sizeof(struct scull_qset));
		if(mdata == NULL)
			return NULL;
		else
		{
			mdata->next = NULL;
			mdata->data = NULL;
		}
	}
	while (node--)
	{
		if((mdata->next) == NULL)
		{
			mdata->next = (struct scull_qset *)malloc(sizeof(struct scull_qset));
			if((mdata->next) == NULL)
				return NULL;
			else
			{
				mdata = mdata->next;
				mdata->next = NULL;
				mdata->data = NULL;
			}
		}
		else
			mdata = mdata->next;
	}
	return mdata;
}

void scull_trim(struct scull_dev *dev)
{
	int i;
	struct scull_qset *mdata;
	struct scull_qset *mnext;
	mdata = dev->data;
	while(mdata != NULL)
	{
		mnext = mdata->next;
		for(i = 0; i < (dev->qset); i++)
		{
			if(mdata->data[i] != NULL)
			{
				free(mdata->data[i]);
			}
			else
			{
				break;
			}
		}
		if(mdata->data != NULL)
		{
			free(mdata->data);
		}
		free(mdata);
		mdata = mnext;
	}
	dev->data = NULL;
	dev->quantum = QUANTUM_SIZE;
	dev->qset = QUANTUMSET_SIZE;
	dev->size = 0;
}

int scull_write(struct scull_dev *dev, char* buff, int count, int *fpos)
{
	struct scull_qset *mdata;
	unsigned long nodesize;
	int quantumnum;
	int nodenum;
	int quantumpos;
	int i;

	nodesize = dev->quantum * dev->qset;

	nodenum = (*fpos)/nodesize;
	quantumnum = ((*fpos)%nodesize) / (dev->quantum);
	quantumpos = ((*fpos)%nodesize) % (dev->quantum);

	mdata = scull_follow(dev, nodenum);

	if(mdata == NULL)
	{
		goto out;
	}
	if(mdata->data == NULL)
	{
		mdata->data = (char **)malloc(sizeof(char *)*(dev->qset));
		if(mdata->data == NULL)
		{
			goto out;
		}
		else
		{
			for(i = 0; i < dev->qset; i++)
			{
				mdata->data[i] = NULL;
			}
		}
	}

	if(mdata->data[quantumnum] == NULL)
	{
		mdata->data[quantumnum] = (char *)calloc(dev->quantum, sizeof(char));
		if(mdata->data[quantumnum] == NULL)
		{
			goto out;
		}
	}
	
	if((dev->quantum - quantumpos) < count)
	{
		count = dev->quantum - quantumpos;
	}
	for(i = 0; i < count; i++)
	{
		mdata->data[quantumnum][quantumpos+i] = buff[i];
	}
	dev->size += count;
	(*fpos) += count;
	return count;
	out:
		printf("Cannot create Quantum node\n");
		return -1;
}

int scull_read(struct scull_dev *dev, char* buff, int count, int *fpos)
{
	struct scull_qset *mdata;
	unsigned long nodesize;
	int nodenum;
	int quantumnum;
	int quantumpos;
	int i;

	if((*fpos) >= dev->size)
	{
		return 0;
	}

	nodesize = dev->quantum * dev->qset;

	nodenum = (*fpos)/nodesize;
	quantumnum = ((*fpos)%nodesize) / (dev->quantum);
	quantumpos = ((*fpos)%nodesize) % (dev->quantum);

	mdata = scull_follow(dev, nodenum);

	if(mdata == NULL || mdata->data == NULL || mdata->data[quantumnum] == NULL)
	{
		goto out;
	}

	if(count > (dev->quantum - quantumpos))
	{
		count = (dev->quantum - quantumpos);
	}

	for(i = 0; i < count; i++)
	{
		//copy data
		buff[i] = mdata->data[quantumnum][quantumpos+i];
	}

	(*fpos) += count;
	return count;
	out:
		printf("Cannot read\n");
		return -1;
}

void main()
{
	char buff[QUANTUM_SIZE + 1];
	struct scull_dev mydev;
	int ret;
	int fpos = 0;
	int fpos2 = 0;
	mydev.data = NULL;
	mydev.quantum = QUANTUM_SIZE;
	mydev.qset = QUANTUMSET_SIZE;
	mydev.size = 0;

	scull_write(&mydev, "Hello", 5, &fpos);
	scull_write(&mydev, "Worldlol", 8, &fpos);
	scull_write(&mydev, "LOLLOLLOL0", 10, &fpos);
	scull_write(&mydev, "LOLLOLLOL1", 10, &fpos);

	ret = scull_read(&mydev, buff, 11, &fpos2);
	if(ret > 0)
	{
		buff[ret] = '\0';
		printf("%s\n", buff);
	}
	ret = scull_read(&mydev, buff, 6, &fpos2);
	if(ret > 0)
	{
		buff[ret] = '\0';
		printf("%s\n", buff);
	}
	ret = scull_read(&mydev, buff, 6, &fpos2);
	if(ret > 0)
	{
		buff[ret] = '\0';
		printf("%s\n", buff);
	}
	ret = scull_read(&mydev, buff, 10, &fpos2);
	if(ret > 0)
	{
		buff[ret] = '\0';
		printf("%s\n", buff);
	}

	scull_trim(&mydev);

	fpos = 0;
	scull_write(&mydev, "Helloab", 7, &fpos);
	fpos2 = 0;
	ret = scull_read(&mydev, buff, 10, &fpos2);
	if(ret > 0)
	{
		buff[ret] = '\0';
		printf("%s\n", buff);
	}

	scull_trim(&mydev);
}