#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include "scull.h"
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/version.h> // Mandatory for version macro

MODULE_LICENSE("GPL");
MODULE_AUTHOR("VinhLV5");
MODULE_DESCRIPTION("scull");
MODULE_VERSION("0.0.2");

int scullminor = 0;
int scullmajor = SCULL_MAJOR;
int scull_nr_dev = SCULL_NR_DEVS;
struct scull_dev *scull_devices = NULL;
int scull_quantum = QUANTUM_SIZE;
int scull_qset = QUANTUMSET_SIZE;
struct class *myclass;


static ssize_t scull_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t scull_write(struct file *, const char __user *, size_t, 
	loff_t *);
static int scull_open(struct inode *, struct file *);
static int scull_release(struct inode *, struct file *);

static struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.open = scull_open,
	.release = scull_release,
	.read = scull_read,
	.write = scull_write,
};

struct scull_qset *scull_follow(struct scull_dev *dev, int node)
{
	struct scull_qset *mdata;
	mdata = dev->data;
	if(mdata == NULL)
	{
		mdata = dev->data = (struct scull_qset *)kmalloc
			(sizeof(struct scull_qset), GFP_KERNEL);
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
			mdata->next = (struct scull_qset *)kmalloc
				(sizeof(struct scull_qset), GFP_KERNEL);
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
				//kfree(mdata->data[i]);
				//kmem_cache_free(quantumqueue, mdata->data[i]);
				free_pages((unsigned long)(mdata->data[i]), dev->quantum);
			}
			else
			{
				break;
			}
		}
		if(mdata->data != NULL)
		{
			kfree(mdata->data);
		}
		kfree(mdata);
		mdata = mnext;
	}
	dev->data = NULL;
	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	dev->size = 0;
}

static int scull_open(struct inode *inodp, struct file *filp)
{
	struct scull_dev *dev;
	dev = container_of(inodp->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev;
	//printk(KERN_ALERT "Before take key in open\n");
	if(down_interruptible(&dev->sem))
	{
		return -ERESTARTSYS;
	}
	//printk(KERN_ALERT "After take key in open\n");
	if((filp->f_flags & O_ACCMODE) == O_WRONLY)
	{
		scull_trim(dev);
	}
	up(&dev->sem);
	return 0;
}

static int scull_release(struct inode *inodp, struct file *filp)
{
	return 0;
}

static ssize_t scull_write(struct file * filp, const char __user * buff, 
	size_t count, loff_t *fpos)
{
	struct scull_dev *dev;
	struct scull_qset *mdata;
	unsigned long nodesize;
	int quantumnum;
	int nodenum;
	int quantumpos;
	int i;
	int retval = -ENOMEM;

	dev = filp->private_data;

	//printk(KERN_ALERT "Before take key in write\n");
	if(down_interruptible(&dev->sem))
	{
		return -ERESTARTSYS;
	}
	//printk(KERN_ALERT "After take key in write\n");

	nodesize = (PAGE_SIZE << (dev->quantum)) * dev->qset;

	nodenum = (long)(*fpos)/nodesize;
	quantumnum = ((long)(*fpos)%nodesize) / (PAGE_SIZE << (dev->quantum));
	quantumpos = ((long)(*fpos)%nodesize) % (PAGE_SIZE << (dev->quantum));

	mdata = scull_follow(dev, nodenum);

	if(mdata == NULL)
	{
		goto out;
	}
	if(mdata->data == NULL)
	{
		mdata->data = (char **)kmalloc(sizeof(char *)*(dev->qset), GFP_KERNEL);
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
		mdata->data[quantumnum] = (char *)__get_free_pages(GFP_KERNEL, dev->quantum);
		if(mdata->data[quantumnum] == NULL)
		{
			goto out;
		}
		memset(mdata->data[quantumnum], 0, (PAGE_SIZE << (dev->quantum)));
	}
	
	if(((PAGE_SIZE << (dev->quantum)) - quantumpos) < count)
	{
		count = (PAGE_SIZE << (dev->quantum)) - quantumpos;
	}

	if(copy_from_user(mdata->data[quantumnum] + quantumpos, buff, count))
	{
		retval = -EFAULT;
		goto out;
	}

	printk(KERN_ALERT "%s with %d char\n", mdata->data[quantumnum], count);

	dev->size += count;
	(*fpos) += count;
	up(&dev->sem);
	retval = count;
	out:
		up(&dev->sem);
		return retval;
}

static ssize_t scull_read(struct file * filp, char __user * buff, size_t count, 
	loff_t *fpos)
{
	struct scull_qset *mdata;
	struct scull_dev *dev;
	unsigned long nodesize;
	int nodenum;
	int quantumnum;
	int quantumpos;
	int retval = 0;

	dev = filp->private_data;

	//printk(KERN_ALERT "before take key in read\n");
	if(down_interruptible(&dev->sem))
	{
		return -ERESTARTSYS;
	}
	//printk(KERN_ALERT "after take key in read\n");

	if((*fpos) >= dev->size)
	{
		goto out;
	}

	nodesize = (PAGE_SIZE << (dev->quantum)) * dev->qset;

	nodenum = (long)(*fpos)/nodesize;
	quantumnum = ((long)(*fpos)%nodesize) / (PAGE_SIZE << (dev->quantum));
	quantumpos = ((long)(*fpos)%nodesize) % (PAGE_SIZE << (dev->quantum));

	mdata = scull_follow(dev, nodenum);

	if(mdata == NULL || mdata->data == NULL || mdata->data[quantumnum] == NULL)
	{
		goto out;
	}

	if(count > ((PAGE_SIZE << (dev->quantum)) - quantumpos))
	{
		count = ((PAGE_SIZE << (dev->quantum)) - quantumpos);
	}
	if(count > (dev->size - (*fpos)))
	{
		count = dev->size - (*fpos);
	}

	if(copy_to_user(buff, mdata->data[quantumnum] + quantumpos, count))
	{
		retval = -EFAULT;
		goto out;
	}

	printk(KERN_ALERT "%s with %d char in read\n", mdata->data[quantumnum], count);

	(*fpos) += count;
	retval = count;
	out:
		up(&dev->sem);
		return retval;
}

static void scull_setup_cdev(struct scull_dev * dev, int index)
{
	int err;
	dev_t scull_dev;
	scull_dev = MKDEV(scullmajor, scullminor + index);
	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_fops;
	err = cdev_add(&dev->cdev, scull_dev, 1);
	if(err)
	{
		printk(KERN_ALERT "Error %d add cdev to scull %d\n", err, index);
	}
}

static int __init scull_init(void)
{
	char buffer[8];
	int ret;
	int i;
	dev_t scull_dev;
	#if(SCULL_MAJOR == 0)
	ret = alloc_chrdev_region(&scull_dev, 0, scull_nr_dev, "scullp");
	#else
	scull_dev = MKDEV(scullmajor, scullminor);
	ret = register_chrdev_region(scull_dev, scull_nr_dev, "scullp");
	#endif
	if(ret < 0)
	{
		goto out;
	}
	scullmajor = MAJOR(scull_dev);
	scullminor = MINOR(scull_dev);

	printk(KERN_ALERT "First major = %d, minor = %d\n", scullmajor, scullminor);

	scull_devices = (struct scull_dev *)kmalloc
		(sizeof(struct scull_dev)*scull_nr_dev, GFP_KERNEL);
	if(scull_devices == NULL)
	{
		goto out2;
	}
	memset(scull_devices, 0, sizeof(struct scull_dev)*scull_nr_dev);

	myclass = class_create(THIS_MODULE, "scullpclass");
	if(myclass == NULL)
	{
		goto out3;
	}

	buffer[7] = '\0';
	for(i = 0; i < scull_nr_dev; i++)
	{
		scull_devices[i].data = NULL;
		scull_devices[i].quantum = scull_quantum;
		scull_devices[i].qset = scull_qset;
		scull_devices[i].size = 0;
		sema_init(&scull_devices[i].sem, 1);
		scull_setup_cdev(&scull_devices[i], i);
		scull_dev = MKDEV(scullmajor, scullminor + i);
		sprintf(buffer, "scullp%d", i);
		device_create(myclass, NULL, scull_dev, NULL, buffer);
	}

	printk(KERN_INFO "page size: %ld\n", PAGE_SIZE);

	return 0;

	out3:
	kfree(scull_devices);
	out2:
	scull_dev = MKDEV(scullmajor, scullminor);
	printk(KERN_ALERT "Cannot malloc scull_devices\n");
	unregister_chrdev_region(scull_dev, scull_nr_dev);
	return -ENOMEM;
	out:
	printk(KERN_WARNING "Cannot create device number\n");
	return -ENOMEM;
}

static void scull_exit(void)
{
	dev_t scull_dev;
	int i;

	for(i = 0; i < scull_nr_dev; i++)
	{
		scull_dev = MKDEV(scullmajor, scullminor + i);
		device_destroy(myclass, scull_dev);
		cdev_del(&scull_devices[i].cdev);
	}
	class_destroy(myclass);

	for(i = 0; i < scull_nr_dev; i++)
	{
		scull_trim(&scull_devices[i]);
	}
	kfree(scull_devices);

	scull_dev = MKDEV(scullmajor, scullminor);
	unregister_chrdev_region(scull_dev, scull_nr_dev);
	printk(KERN_ALERT "Goodbyte\n");
}

module_init(scull_init);
module_exit(scull_exit);