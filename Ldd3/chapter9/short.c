#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/errno.h>
//#include <asm/system.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("VINH");

unsigned int *vitual;

#define PARALLEL_BASE 	0x3F000000
#define GPIO_SEL_BASE 			(PARALLEL_BASE + 0x00200004)
#define GPIO_OUT_BASE				(PARALLEL_BASE + 0x0020001C)
#define GPIO_CLR_BASE				(PARALLEL_BASE + 0x00200028)
#define NR_OF_DEVICE		2

// unsigned long short_base;
// dev_t short_dev;
// int shortmajor, shortminor;
// struct cdev cdev[NR_OF_DEVICE];
// struct class *short_class;

unsigned int portsel, portset, portclr;

// static ssize_t short_read(struct file *, char __user *, size_t, loff_t *);
// static ssize_t short_write(struct file *, const char __user *, size_t, 
// 	loff_t *);
// static int short_open(struct inode *, struct file *);
// static int short_close(struct inode *, struct file *);

// struct file_operations short_fops = {
// 	.owner = THIS_MODULE,
// 	.open = short_open,
// 	.read = short_read,
// 	.write = short_write,
// 	.release = short_close,
// };

// static ssize_t short_read(struct file *filp, char __user *buff, size_t count, loff_t *fpos)
// {
// 	return 0;
// }
// static ssize_t short_write(struct file *filp, const char __user *buff, size_t count, loff_t *fpos)
// {
// 	return 0;
// }
// static int short_open(struct inode *inodp, struct file *filp)
// {
// 	return 0;
// }
// static int short_close(struct inode *inodp, struct file *filp)
// {
// 	return 0;
// }

static int short_init(void)
{
	// int i, j;
	// char buffer[7]; 
	// dev_t temp_dev;
	unsigned int temp;

	printk(KERN_INFO "Hello, ulong = %d\n", sizeof(unsigned long));
	printk(KERN_INFO "Hello, uint = %d\n", sizeof(unsigned int));

	portsel = GPIO_SEL_BASE;
	portset = GPIO_OUT_BASE;
	portclr = GPIO_CLR_BASE;

	if(request_region(GPIO_SEL_BASE, 1, "short"))
	{
	 	printk(KERN_INFO "Cannot request sel port\n");
	 	return -ENOMEM;
	}


	if(request_region(GPIO_OUT_BASE, 1, "short"))
	{
		release_region(portsel, 1);
		printk(KERN_INFO "Cannot request set port\n");
		return -ENOMEM;
	}


	if(request_region(GPIO_CLR_BASE, 1, "short"))
	{
		release_region(portsel, 1);
		release_region(portset, 1);
		printk(KERN_INFO "Cannot request clr port\n");
		return -ENOMEM;
	}

	temp = inl(portsel);
	rmb();
	outl((temp | ((unsigned int)1<<15)), portsel);
	wmb();
	outl(((unsigned int)1<<15), portset);
	wmb();
	// if(request_region(GPIO_BASE, NR_OF_DEVICE, "short"))
	// {
	// 	printk(KERN_INFO "Cannot request io for device\n");
	// 	return -ENOMEM;
	// }

	// short_base = (unsigned long)ioremap(GPIO_BASE, NR_OF_DEVICE);

	// if(alloc_chrdev_region(&short_dev, 0, NR_OF_DEVICE, "short"))
	// {
	// 	printk(KERN_INFO "Cannot request device number for driver\n");
	// 	release_region(short_base, NR_OF_DEVICE);
	// 	return -ENOMEM;
	// }
	// shortmajor = MAJOR(short_dev); shortminor = MINOR(short_dev);
	// short_class = class_create(THIS_MODULE, "short_class");
	// if(short_class == NULL)
	// {
	// 	goto out;
	// }

	// for(i = 0; i < NR_OF_DEVICE; i++)
	// {
	// 	memset(buffer, 0, 7);
	// 	sprintf(buffer, "short%d", i);
	// 	temp_dev = MKDEV(shortmajor, shortminor+i);
	// 	cdev_init(&cdev[i], &short_fops);
	// 	cdev[i].owner = THIS_MODULE;
	// 	cdev[i].ops = &short_fops;
	// 	if(cdev_add(&cdev[i], temp_dev, 1))
	// 	{
	// 		goto out1;
	// 	}
	// 	device_create(short_class, NULL, temp_dev, NULL, buffer);
	// }



	printk(KERN_INFO "Init successfully\n");
	// return 0;
	// out1:
	// for(j = 0; j < i; j++)
	// {
	// 	temp_dev = MKDEV(shortmajor, shortminor+i);
	// 	device_destroy(short_class, temp_dev);
	// 	cdev_del(&cdev[j]);
	// }
	// class_destroy(short_class);
	// out:
	// release_region(short_base, NR_OF_DEVICE);
	// unregister_chrdev_region(short_dev, NR_OF_DEVICE);
	// return -ENOMEM;
	return 0;
}

static void short_exit(void)
{
	//unsigned int a;
	// int i;
	// dev_t temp_dev;
	// for(i = 0; i < NR_OF_DEVICE; i++)
	// {
	// 	temp_dev = MKDEV(shortmajor, shortminor+i);
	// 	device_destroy(short_class, temp_dev);
	// 	cdev_del(&cdev[i]);
	// }
	// class_destroy(short_class);
	// release_region(short_base, NR_OF_DEVICE);
	// unregister_chrdev_region(short_dev, NR_OF_DEVICE
	//a = (unsigned int)&vitual[10]abc
	outl(((unsigned int)1<<15), portclr);
	wmb();

	release_region(portsel, 1);
	release_region(portset, 1);
	release_region(portclr, 1);
	printk(KERN_INFO "Goodbye\n");
}

module_init(short_init);
module_exit(short_exit);