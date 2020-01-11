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

#define PARALLEL_BASE                   0x3F8
#define NR_OF_DEVICE					1

static int short_init(void)
{
	unsigned int temp;
	if(request_region(PARALLEL_BASE, 5, "short2") == NULL)
	{
		printk(KERN_INFO "Cannot request io for device\n");
		return -ENOMEM;
	}

	// parallel port is not available on x64 system

	outb(0x06,PARALLEL_BASE);

	printk("Value out put: %d\n", inb(PARALLEL_BASE));

	return 0;
}

static void short_exit(void)
{
	release_region(PARALLEL_BASE, 5);
	printk(KERN_INFO "Goodbye\n");
}

module_init(short_init);
module_exit(short_exit);