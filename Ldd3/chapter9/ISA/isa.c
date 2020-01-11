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

#define ISA_BEGIN		0xA0000
#define ISA_MAX			0xA0100

#define NR_OF_DEVICE					2

// dev_t isa_dev;
// int isamajor, isaminor;
// struct cdev cdev;
// struct class *isa_class;
void *isa_base;

// static ssize_t isa_read(struct file *, char __user *, size_t, loff_t *);
// static ssize_t isa_write(struct file *, const char __user *, size_t, 
// 	loff_t *);
// static int isa_open(struct inode *, struct file *);
// static int isa_close(struct inode *, struct file *);

// struct file_operations isa_fops = {
// 	.owner = THIS_MODULE,
// 	.open = isa_open,
// 	.read = isa_read,
// 	.write = isa_write,
// 	.release = isa_close,
// };

// static ssize_t isa_read(struct file *filp, char __user *buff, size_t count, loff_t *fpos)
// {
// 	char kerbuff[5] = {0};
// 	if((ioread32(GPIO_INP_BASE)&((unsigned int)1<<15)))
// 	{
// 		strcpy(kerbuff, "HIGH");
// 		if(count > 5)
// 			count = 5;
// 	}
// 	else
// 	{
// 		strcpy(kerbuff, "LOW");
// 		if(count > 4)
// 			count = 4;
// 	}
// 	if(copy_to_user(buff, kerbuff, count))
// 	{
// 		return -EFAULT;
// 	}
// 	return count;
// }
// static ssize_t isa_write(struct file *filp, const char __user *buff, size_t count, loff_t *fpos)
// {
// 	char kerbuff[2] = {0};
// 	if(count > 0)
// 	{
// 		if(count > 1)
// 			count = 1;
// 		if(copy_from_user(kerbuff, buff, 1))
// 		{
// 			return -EFAULT;
// 		}
// 		switch(kerbuff[0])
// 		{
// 			case '0':
// 				iowrite32(((unsigned int)1<<15), GPIO_CLR_BASE);
// 				break;
// 			default:
// 				iowrite32(((unsigned int)1<<15), GPIO_OUT_BASE);
// 				break;
// 		}
// 	}
// 	return count;
// }
// static int isa_open(struct inode *inodp, struct file *filp)
// {
// 	return 0;
// }
// static int isa_close(struct inode *inodp, struct file *filp)
// {
// 	return 0;
// }

static int isa_init(void)
{
	// unsigned int temp;
	// if(request_mem_region(ISA_BEGIN, ISA_MAX - ISA_BEGIN, "isa") == NULL)
	// {
	// 	printk(KERN_INFO "Cannot request io for device\n");
	// 	return -ENOMEM;
	// }
	// cannot map ISA memory to kernel

	// cannot modify isa memory on x64 processor

	isa_base = ioremap(ISA_BEGIN, ISA_MAX - ISA_BEGIN);

	if(isa_base == NULL)
	{
		printk(KERN_INFO "Cannot remap isa memory\n");
		return -ENOMEM;
	}

	iowrite32(0x04030201, isa_base);
	printk(KERN_INFO "low byte:%d\n", ioread8(isa_base));
	printk(KERN_INFO "high byte: %d\n", ioread8(isa_base + 1));
	printk(KERN_INFO "Value32 = %d\n", ioread32(isa_base));

	iowrite8(0x06, isa_base+4);
	printk(KERN_INFO "Value8: %d\n", ioread8(isa_base+4));

	printk(KERN_INFO "Mapping ok\n");
	return 0;
	// isa_base = ioremap(PARALLEL_BASE, 100);
	// if(isa_base == NULL)
	// {
	// 	printk(KERN_INFO "Cannot map io region\n");
	// 	return -ENOMEM;
	// }

	// if(alloc_chrdev_region(&isa_dev, 0, NR_OF_DEVICE, "short"))
	// {
	// 	printk(KERN_INFO "Cannot request device number for driver\n");
	// 	iounmap(isa_base);
	// 	return -ENOMEM;
	// }
	// isamajor = MAJOR(isa_dev); isaminor = MINOR(isa_dev);
	// isa_class = class_create(THIS_MODULE, "isa_class");
	// if(isa_class == NULL)
	// {
	// 	goto out;
	// }

	// cdev_init(&cdev, &isa_fops);
	// cdev.owner = THIS_MODULE;
	// cdev.ops = &isa_fops;
	// if(cdev_add(&cdev, isa_dev, 1))
	// {
	// 	goto out1;
	// }
	// device_create(isa_class, NULL, isa_dev, NULL, "short0");

	// temp = ioread32(GPIO_SEL_BASE);
	// iowrite32(temp | ((unsigned int)1<<15), GPIO_SEL_BASE);
	// iowrite32(((unsigned int)1<<15), GPIO_OUT_BASE);

	// printk(KERN_INFO "Init ok\n");
	// return 0;

	// out1:
	// class_destroy(isa_class);
	// out:
	// iounmap(isa_base);
	// unregister_chrdev_region(isa_dev, NR_OF_DEVICE);
	// return -ENOMEM;
}

static void isa_exit(void)
{
	// device_destroy(isa_class, isa_dev);
	// cdev_del(&cdev);
	// class_destroy(isa_class);
	// unregister_chrdev_region(isa_dev, NR_OF_DEVICE);
	// iowrite32(((unsigned int)1<<15), GPIO_CLR_BASE);
	// iounmap(isa_base);
	//release_mem_region(ISA_BEGIN, ISA_MAX - ISA_BEGIN);
	iounmap(isa_base);
	printk(KERN_INFO "Goodbye\n");
}

module_init(isa_init);
module_exit(isa_exit);