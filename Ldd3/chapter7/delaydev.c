#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinhasdf");

#define BUFFER_SIZE 30

struct delay_dev{
	wait_queue_head_t wait;
	struct timer_list tim;
	struct semaphore sem;
	char *buffer;
	struct cdev cdev;
};

static int delay_open(struct inode *, struct file *);
static int delay_close(struct inode *, struct file *);
static ssize_t delay_write(struct file *, const char __user *, size_t, 
	loff_t *);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = delay_open,
	.release = delay_close,
	.write = delay_write,
};

DECLARE_WAIT_QUEUE_HEAD(quit_wait);
int numberaccess = 0;
bool isshutdown = false;
dev_t delay_dev;
struct delay_dev *mdev = NULL;
struct class *myclass;
struct workqueue_struct *myworkqueue;
struct work_struct mywork;

static int delay_open(struct inode *inodp, struct file *filp)
{
	struct delay_dev *dev;
	dev = container_of(inodp->i_cdev, struct delay_dev, cdev);
	if(dev != NULL)
	{
		if(down_interruptible(&dev->sem))
		{
			return -ERESTARTSYS;
		}
		numberaccess++;
		if(dev->buffer == NULL)
		{
			dev->buffer = (char *)kmalloc(sizeof(char)*BUFFER_SIZE, GFP_KERNEL);
			if(dev->buffer == NULL)
			{
				numberaccess--;
				up(&dev->sem);
				return -ENOMEM;
			}
		}
		up(&dev->sem);
		filp->private_data = dev;
		printk(KERN_INFO "Enter open\n");
	}
	else
	{
		return -ENOMEM;
	}
	return 0;
}

static int delay_close(struct inode *inodp, struct file *filp)
{
	struct delay_dev *dev;
	dev = filp->private_data;
	if(down_interruptible(&dev->sem))
	{
		return -ERESTARTSYS;
	}
	if(dev->buffer != NULL)
		kfree(dev->buffer);
	numberaccess--;
	up(&dev->sem);
	if(numberaccess == 0)
	{
		wake_up_interruptible(&quit_wait);
	}
	return 0;
}

void callback_func(struct timer_list *arg)
{
	struct delay_dev *dev;
	dev = container_of(arg, struct delay_dev, tim);
	printk(KERN_INFO "buff: %s, is_in_interrupt: %i, cpu: %i jiffies: %9li\n", 
		dev->buffer, in_interrupt() ? 1:0, smp_processor_id(), jiffies);
	wake_up_interruptible(&dev->wait);
	printk(KERN_INFO "Wake up process from timer\n");
	isshutdown = true;
}

void callback_work(struct work_struct *arg)
{
	ssleep(1);
	printk(KERN_INFO "is_in_interrupt: %i, cpu: %i jiffies: %9li\n", 
		in_interrupt() ? 1:0, smp_processor_id(), jiffies);
	wake_up_interruptible(&mdev->wait);
	printk(KERN_INFO "Wake up process from workqueue\n");
	isshutdown = true;
}

static ssize_t delay_write(struct file *filp, const char __user *buff, size_t count, 
	loff_t *fpos)
{
	struct delay_dev *dev;
	dev = filp->private_data;
	if(count > BUFFER_SIZE)
		count = BUFFER_SIZE;
	if(down_interruptible(&dev->sem))
	{
		return -ERESTARTSYS;
	}
	if(copy_from_user(dev->buffer, buff, count))
	{
		up(&dev->sem);
		return -EFAULT;
	}
	
	printk(KERN_INFO "jiffies: %9li\n", jiffies);
	if(dev->buffer[0] == '0')
	{
		dev->tim.expires = jiffies + HZ;
		//dev->tim.data = dev;
		add_timer(&dev->tim);
		up(&dev->sem);
		wait_event_interruptible(dev->wait, isshutdown);
		printk(KERN_INFO "Out of func\n");
		isshutdown = false;
	}
	else
	{
		// enter workqueue
		queue_work(myworkqueue, &mywork);
		up(&dev->sem);
		wait_event_interruptible(dev->wait, isshutdown);
		isshutdown = false;
	}
	return count;
}

static int __init delay_init(void)
{
	int ret;
	ret = alloc_chrdev_region(&delay_dev, 0, 1, "delay_dev");
	if(ret < 0)
		return -ENOMEM;
	mdev = (struct delay_dev *)kmalloc(sizeof(struct delay_dev), GFP_KERNEL);
	if(mdev != NULL)
	{
		mdev->buffer = NULL;
		sema_init(&mdev->sem, 1);
		init_waitqueue_head(&mdev->wait);
		cdev_init(&mdev->cdev, &fops);
		timer_setup(&mdev->tim, callback_func, 0);
		mdev->cdev.owner = THIS_MODULE;
		mdev->cdev.ops = &fops;
		if(cdev_add(&mdev->cdev, delay_dev, 1))
		{
			kfree(mdev);
			unregister_chrdev_region(delay_dev, 1);
			return -ENOMEM;
		}
		myclass = class_create(THIS_MODULE, "delay_class");
		if(myclass != NULL)
		{
			printk(KERN_INFO "Init device\n");
			device_create(myclass, NULL, delay_dev, NULL, "delay_dev0");
			myworkqueue = create_workqueue("myworkqueue");
			INIT_WORK(&mywork, callback_work);
		}
		else
		{
			printk(KERN_INFO "Clear device, fail to init\n");
			del_timer_sync(&mdev->tim);
			cdev_del(&mdev->cdev);
			kfree(mdev);
			unregister_chrdev_region(delay_dev, 1);
		}

	}
	else
	{
		return -ENOMEM;
	}
	return 0;
}

static void __exit delay_exit(void)
{
	wait_event_interruptible(quit_wait, numberaccess == 0);
	isshutdown = true;
	device_destroy(myclass, delay_dev);
	class_destroy(myclass);
	del_timer_sync(&mdev->tim);
	flush_workqueue(myworkqueue);
	destroy_workqueue(myworkqueue);
	cdev_del(&mdev->cdev);
	kfree(mdev);
	unregister_chrdev_region(delay_dev, 1);
}

module_init(delay_init);
module_exit(delay_exit);
