#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/errno.h>
//#include <linux/system.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("VINH");

#define PARALLEL_BASE                   0x3F200000
#define GPIO_SEL_BASE                   ((unsigned char *)short_base + 0x00000004)
#define GPIO_OUT_BASE                   (short_base + 0x0000001C)
#define GPIO_CLR_BASE					(short_base + 0x00000028)
#define GPIO_INP_BASE					(short_base + 0x00000034)

#define NR_OF_DEVICE					1

dev_t short_dev;
int shortmajor, shortminor;
struct cdev cdev;
struct class *short_class;
void *short_base;

static ssize_t short_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t short_write(struct file *, const char __user *, size_t, 
	loff_t *);
static int short_open(struct inode *, struct file *);
static int short_close(struct inode *, struct file *);

struct file_operations short_fops = {
	.owner = THIS_MODULE,
	.open = short_open,
	.read = short_read,
	.write = short_write,
	.release = short_close,
};

static ssize_t short_read(struct file *filp, char __user *buff, size_t count, loff_t *fpos)
{
	char kerbuff[5] = {0};
	if((ioread32(GPIO_INP_BASE)&((unsigned int)1<<15)))
	{
		strcpy(kerbuff, "HIGH");
		if(count > 5)
			count = 5;
	}
	else
	{
		strcpy(kerbuff, "LOW");
		if(count > 4)
			count = 4;
	}
	if(copy_to_user(buff, kerbuff, count))
	{
		return -EFAULT;
	}
	return count;
}
static ssize_t short_write(struct file *filp, const char __user *buff, size_t count, loff_t *fpos)
{
	char kerbuff[2] = {0};
	if(count > 0)
	{
		if(count > 1)
			count = 1;
		if(copy_from_user(kerbuff, buff, 1))
		{
			return -EFAULT;
		}
		switch(kerbuff[0])
		{
			case '0':
				iowrite32(((unsigned int)1<<15), GPIO_CLR_BASE);
				break;
			default:
				iowrite32(((unsigned int)1<<15), GPIO_OUT_BASE);
				break;
		}
	}
	return count;
}
static int short_open(struct inode *inodp, struct file *filp)
{
	return 0;
}
static int short_close(struct inode *inodp, struct file *filp)
{
	return 0;
}

static int short_init(void)
{
	unsigned int temp;
	// if(request_mem_region(PARALLEL_BASE, 1, "short") == NULL)
	// {
	// 	printk(KERN_INFO "Cannot request io for device\n");
	// 	return -ENOMEM;
	// }

	short_base = ioremap(PARALLEL_BASE, 100);
	if(short_base == NULL)
	{
		printk(KERN_INFO "Cannot map io region\n");
		return -ENOMEM;
	}

	if(alloc_chrdev_region(&short_dev, 0, NR_OF_DEVICE, "short"))
	{
		printk(KERN_INFO "Cannot request device number for driver\n");
		iounmap(short_base);
		return -ENOMEM;
	}
	shortmajor = MAJOR(short_dev); shortminor = MINOR(short_dev);
	short_class = class_create(THIS_MODULE, "short_class");
	if(short_class == NULL)
	{
		goto out;
	}

	cdev_init(&cdev, &short_fops);
	cdev.owner = THIS_MODULE;
	cdev.ops = &short_fops;
	if(cdev_add(&cdev, short_dev, 1))
	{
		goto out1;
	}
	device_create(short_class, NULL, short_dev, NULL, "short0");

	temp = ioread32(GPIO_SEL_BASE);
	iowrite32(temp | ((unsigned int)1<<15), GPIO_SEL_BASE);
	iowrite32(((unsigned int)1<<15), GPIO_OUT_BASE);

	printk(KERN_INFO "Init ok\n");
	return 0;

	out1:
	class_destroy(short_class);
	out:
	iounmap(short_base);
	unregister_chrdev_region(short_dev, NR_OF_DEVICE);
	return -ENOMEM;
}

static void short_exit(void)
{
	device_destroy(short_class, short_dev);
	cdev_del(&cdev);
	class_destroy(short_class);
	unregister_chrdev_region(short_dev, NR_OF_DEVICE);
	iowrite32(((unsigned int)1<<15), GPIO_CLR_BASE);
	iounmap(short_base);
	printk(KERN_INFO "Goodbye\n");
}

module_init(short_init);
module_exit(short_exit);