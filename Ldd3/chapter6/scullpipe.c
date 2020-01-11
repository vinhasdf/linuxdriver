#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/version.h> // Mandatory for version macro
#include "scullpipe.h"
#include <linux/sched/signal.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinhasdf");

int scull_numofdevices = SCP_NUM_DEVICES;
int scull_buffersize = SCP_DEFAULT_BUFFER_SIZE;
dev_t scull_dev;
struct scull_pipe *scull_devices = NULL;

static ssize_t scull_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t scull_write(struct file *, const char __user *, size_t, 
	loff_t *);
static int scull_open(struct inode *, struct file *);
static int scull_release(struct inode *, struct file *);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = scull_open,
	.release = scull_release,
	.write = scull_write,
	.read = scull_read,
};

static int scull_open(struct inode *inodp, struct file *filp)
{
	int retval = 0;
	struct scull_pipe *dev;
	dev = container_of(inodp->i_cdev, struct scull_pipe, cdev);
	if(down_interruptible(&dev->sem))
	{
		return -ERESTARTSYS;
	}

	if(dev->beginbuff == NULL)
	{
		printk(KERN_INFO "Process %s init the data\n", current->comm);
		dev->beginbuff = (char *)kmalloc(sizeof(char)*scull_buffersize, 
			GFP_KERNEL);
		if(dev->beginbuff == NULL)
		{
			retval = -ENOMEM;
		}
		else
		{
			dev->buffer = scull_buffersize;
			dev->wp = dev->rp = dev->beginbuff;
			dev->endbuff = dev->beginbuff + dev->buffer;
		}
	}
	filp->private_data = dev;
	up(&dev->sem);
	if(retval == 0)
	{
		printk(KERN_INFO "Process %s return successfull\n", current->comm);
	}
	return retval;
}

static int scull_release(struct inode *inodp, struct file *filp)
{
	return 0;
}

static ssize_t scull_read(struct file *filp, char __user *buff, size_t count, loff_t *fpos)
{
	struct scull_pipe *dev;
	int retval;
	dev = filp->private_data;
	if(down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if(dev->wp == dev->rp)
	{
		up(&dev->sem);
		if((filp->f_flags & O_NONBLOCK) == O_NONBLOCK)
			return -EAGAIN;
		if(wait_event_interruptible(dev->rqu, (dev->rp != dev->wp)))
			return -ERESTARTSYS;
		if(down_interruptible(&dev->sem))
			return -ERESTARTSYS;
	}

	if(count > (dev->endbuff - dev->rp))
	{
		count = dev->endbuff - dev->rp;
	}
	if((dev->wp > dev->rp)&&(count > (dev->wp - dev->rp)))
	{
		count = dev->wp - dev->rp;
	}

	if(copy_to_user(buff, dev->rp, count))
	{
		retval = -EFAULT;
	}
	else
	{
		retval = count;
		dev->rp += count;
		if(dev->rp == dev->endbuff)
			dev->rp = dev->beginbuff;
		wake_up_interruptible(&dev->wqu);
	}
	up(&dev->sem);
	
	return retval;
}

static int scull_p_space(struct scull_pipe *dev)
{
	int retval;
	if(dev->rp == dev->wp)
	{
		retval = dev->buffer - 1;
	}
	else
	{
		retval = ((dev->rp + dev->buffer - dev->wp) % (dev->buffer)) - 1;
		if(retval > (dev->endbuff - dev->wp))
			retval = dev->endbuff - dev->wp;
	}
	return retval;
}

static int scull_write_wait(struct scull_pipe *dev, struct file *filp)
{
	DEFINE_WAIT(mywait);
	while(scull_p_space(dev) == 0)
	{
		up(&dev->sem);
		if((filp->f_flags & O_NONBLOCK) == O_NONBLOCK)
			return -EAGAIN;
		prepare_to_wait(&dev->wqu, &mywait, TASK_INTERRUPTIBLE); // change state of process to SLEEP
		if(scull_p_space(dev) == 0)
		{
			schedule(); // YEILD process
		}
		finish_wait(&dev->wqu, &mywait); // clean wait from wait queue head
		if(signal_pending(current)) // if is interrupt from process
			return -ERESTARTSYS;
		if(down_interruptible(&dev->sem))
			return -ERESTARTSYS;
	}
	return 0;
}

static ssize_t scull_write(struct file *filp, const char __user *buff, size_t count, 
	loff_t *fpos)
{
	struct scull_pipe *dev;
	int retval, Lscull_size;
	dev = filp->private_data;
	if(down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	retval = scull_write_wait(dev, filp);
	if(retval)
		return retval;
	printk(KERN_INFO "Point 1\n");
	Lscull_size = scull_p_space(dev);
	if(count > Lscull_size)
		count = Lscull_size;

	printk(KERN_INFO "Point 2: %d\n", count);

	if(copy_from_user(dev->wp, buff, count))
	{
		retval = -EFAULT;
	}
	else
	{
		retval = count;
		dev->wp += count;
		if(dev->wp == dev->endbuff)
			dev->wp = dev->beginbuff;
		wake_up_interruptible(&dev->rqu);
	}
	up(&dev->sem);
	return retval;
}

static void scull_setup_cdev(struct scull_pipe *dev, int index)
{
	int scull_major, scull_minor, err;
	dev_t tempscull;
	scull_major = MAJOR(scull_dev);
	scull_minor = MINOR(scull_dev);
	tempscull = MKDEV(scull_major, scull_minor + index);
	cdev_init(&dev[index].cdev, &fops);
	dev[index].cdev.owner = THIS_MODULE;
	dev[index].cdev.ops = &fops;
	err = cdev_add(&dev[index].cdev, tempscull, 1);
	if(err)
		printk(KERN_INFO "Cannot add cdev to device %i\n", index);
}

static int __init scullpipe_init(void)
{
	int ret, i;
	ret = alloc_chrdev_region(&scull_dev, 0, scull_numofdevices, "scullpipe");
	if(ret < 0)
	{
		return -ENOMEM;
	}
	scull_devices = (struct scull_pipe *)kmalloc(
		sizeof(struct scull_pipe)*scull_numofdevices, GFP_KERNEL);
	if(scull_devices != NULL)
	{
		for(i = 0; i < scull_numofdevices; i++)
		{
			init_waitqueue_head(&scull_devices[i].rqu);
			init_waitqueue_head(&scull_devices[i].wqu);
			sema_init(&scull_devices[i].sem, 1);
			scull_devices[i].beginbuff = NULL;
			scull_devices[i].endbuff = NULL;
			scull_devices[i].rp = NULL;
			scull_devices[i].wp = NULL;
			scull_setup_cdev(scull_devices, i);
		}
	}
	else
	{
		unregister_chrdev_region(scull_dev, scull_numofdevices);
		return -ENOMEM;
	}
	return scull_numofdevices;
}

static void __exit scullpipe_exit(void)
{
	int i;
	for(i = 0; i < scull_numofdevices; i++)
	{
		cdev_del(&scull_devices[i].cdev);
		if(scull_devices[i].beginbuff != NULL)
			kfree(scull_devices[i].beginbuff);
	}
	kfree(scull_devices);
	unregister_chrdev_region(scull_dev, scull_numofdevices);
}

module_init(scullpipe_init);
module_exit(scullpipe_exit);
