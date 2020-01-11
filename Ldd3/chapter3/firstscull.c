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

static ssize_t scull_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t scull_write(struct file *, const char __user *, size_t, 
	loff_t *);
static int scull_open(struct inode *, struct file *);
static int scull_release(struct inode *, struct file *);

#if(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 39))
static int scull_ioctl(struct inode *, struct file *, unsigned int, 
	unsigned long);
#else
static long scull_ioctl(struct file *, unsigned int, unsigned long);
#endif

static struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.open = scull_open,
	.release = scull_release,
	.read = scull_read,
	.write = scull_write,
	#if(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 39))
	.ioctl = scull_ioctl,
	#else
	.unlocked_ioctl = scull_ioctl,
	#endif
};

#if(LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 39))
static int scull_ioctl(struct inode *inodp, struct file *filp, unsigned int cmd,
	unsigned long arg)
#else
static long scull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	int Luslaccess = 1;
	int retval = 0;
	int temp;
	//capable - Determine if the current task has a superior capability in effect
	if(_IOC_DIR(cmd) == _IOC_WRITE)
	{
		// check access argument
		Luslaccess = access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	}
	else if(_IOC_DIR(cmd) == _IOC_READ)
	{
		Luslaccess = access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}

	if(Luslaccess == 0)
	{
		return -EFAULT;
	}

	switch(cmd)
	{
		case SCULL_IOCRESET:
			scull_quantum = QUANTUM_SIZE;
			scull_qset = QUANTUMSET_SIZE;
			retval = 0;
		break;
		
		case SCULL_IOCSQUANTUM:
			if(!capable(CAP_SYS_ADMIN))
			{
				return -EPERM;
			}
			retval = __get_user(scull_quantum, (int __user *)arg);
		break;

		case SCULL_IOCSQSET:
			if(!capable(CAP_SYS_ADMIN))
			{
				return -EPERM;
			}
			retval = __get_user(scull_qset, (int __user *)arg);
		break;

		case SCULL_IOCTQUANTUM:
			if(!capable(CAP_SYS_ADMIN))
			{
				return -EPERM;
			}
			scull_quantum = (int)arg;
		break;

		case SCULL_IOCTQSET:
			if(!capable(CAP_SYS_ADMIN))
			{
				return -EPERM;
			}
			scull_qset = (int)arg;
		break;

		case SCULL_IOCGQUANTUM:
			retval = __put_user(scull_quantum, (int __user *)arg);
		break;

		case SCULL_IOCGQSET:
			retval = __put_user(scull_qset, (int __user *)arg);
		break;

		case SCULL_IOCQQUANTUM:
			retval = scull_quantum;
		break;

		case SCULL_IOCQQSET:
			retval = scull_qset;
		break;

		case SCULL_IOCXQUANTUM:
			if(!capable(CAP_SYS_ADMIN))
			{
				return -EPERM;
			}
			retval = __get_user(temp, (int __user *)arg);
			if(retval != 0)
			{
				return retval;
			}
			retval = __put_user(scull_quantum, (int __user *)arg);
			if(retval != 0)
			{
				return retval;
			}
			scull_quantum = temp;
		break;

		case SCULL_IOCXQSET:
			if(!capable(CAP_SYS_ADMIN))
			{
				return -EPERM;
			}
			retval = __get_user(temp, (int __user *)arg);
			if(retval != 0)
			{
				return retval;
			}
			retval = __put_user(scull_qset, (int __user *)arg);
			if(retval != 0)
			{
				return retval;
			}
			scull_qset = temp;
		break;

		case SCULL_IOCHQUANTUM:
			if(!capable(CAP_SYS_ADMIN))
			{
				return -EPERM;
			}
			temp = (int)arg;
			retval = scull_quantum;
			scull_quantum = temp;
		break;

		case SCULL_IOCHQSET:
			temp = (int)arg;
			retval = scull_qset;
			scull_qset = temp;
		break;

		default:
			// invalid cmd
			retval = -EINVAL;
		break;
	}
	return retval;
}

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
				kfree(mdata->data[i]);
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
	printk(KERN_ALERT "Before take key in open\n");
	if(down_interruptible(&dev->sem))
	{
		return -ERESTARTSYS;
	}
	printk(KERN_ALERT "After take key in open\n");
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

	printk(KERN_ALERT "Before take key in write\n");
	if(down_interruptible(&dev->sem))
	{
		return -ERESTARTSYS;
	}
	printk(KERN_ALERT "After take key in write\n");

	nodesize = dev->quantum * dev->qset;

	nodenum = (long)(*fpos)/nodesize;
	quantumnum = ((long)(*fpos)%nodesize) / (dev->quantum);
	quantumpos = ((long)(*fpos)%nodesize) % (dev->quantum);

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
		mdata->data[quantumnum] = (char *)kmalloc
			((dev->quantum)*(sizeof(char)), GFP_KERNEL);
		if(mdata->data[quantumnum] == NULL)
		{
			goto out;
		}
	}
	
	if((dev->quantum - quantumpos) < count)
	{
		count = dev->quantum - quantumpos;
	}

	if(copy_from_user(mdata->data[quantumnum] + quantumpos, buff, count))
	{
		retval = -EFAULT;
		goto out;
	}

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

	printk(KERN_ALERT "before take key in read\n");
	if(down_interruptible(&dev->sem))
	{
		return -ERESTARTSYS;
	}
	printk(KERN_ALERT "after take key in read\n");

	if((*fpos) >= dev->size)
	{
		goto out;
	}

	nodesize = dev->quantum * dev->qset;

	nodenum = (long)(*fpos)/nodesize;
	quantumnum = ((long)(*fpos)%nodesize) / (dev->quantum);
	quantumpos = ((long)(*fpos)%nodesize) % (dev->quantum);

	mdata = scull_follow(dev, nodenum);

	if(mdata == NULL || mdata->data == NULL || mdata->data[quantumnum] == NULL)
	{
		goto out;
	}

	if(count > (dev->quantum - quantumpos))
	{
		count = (dev->quantum - quantumpos);
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
	int ret;
	int i;
	dev_t scull_dev;
	#if(SCULL_MAJOR == 0)
	ret = alloc_chrdev_region(&scull_dev, 0, scull_nr_dev, "scull");
	#else
	scull_dev = MKDEV(scullmajor, scullminor);
	ret = register_chrdev_region(scull_dev, scull_nr_dev, "scull");
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
	for(i = 0; i < scull_nr_dev; i++)
	{
		scull_devices[i].data = NULL;
		scull_devices[i].quantum = scull_quantum;
		scull_devices[i].qset = scull_qset;
		scull_devices[i].size = 0;
		sema_init(&scull_devices[i].sem, 1);
		scull_setup_cdev(&scull_devices[i], i);
	}

	return 0;
	out:
	printk(KERN_WARNING "Cannot create device number\n");
	return -ENOMEM;
	out2:
	printk(KERN_ALERT "Cannot malloc scull_devices\n");
	unregister_chrdev_region(scull_dev, scull_nr_dev);
	return -ENOMEM;
}

static void __exit scull_exit(void)
{
	dev_t scull_dev;
	int i;

	for(i = 0; i < scull_nr_dev; i++)
	{
		cdev_del(&scull_devices[i].cdev);
	}

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