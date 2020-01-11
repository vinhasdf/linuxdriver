#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/completion.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinhasdf");
MODULE_VERSION("0.0.1");

struct cdev cdev;
dev_t mydev;
DECLARE_COMPLETION(mycomple);

static ssize_t comple_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t comple_write(struct file *, const char __user *, 
	size_t, loff_t *);
static int comple_open(struct inode *, struct file *);
static int comple_release(struct inode *, struct file *);

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = comple_open,
	.release = comple_release,
	.read = comple_read,
	.write = comple_write,
};

static ssize_t comple_read(struct file *filp, char __user *buff, size_t count,
	loff_t *fpos)
{
	printk(KERN_INFO "Read request from process: %s with pid: %d", 
		current->comm, current->pid);
	// wait for complete
	wait_for_completion(&mycomple);
	printk(KERN_INFO "Wake up from process: %s with pid: %d", current->comm,
		current->pid);
	return count;
}
static ssize_t comple_write(struct file *filp, const char __user *buff, 
	size_t count, loff_t *fpos)
{
	printk(KERN_INFO "Write request from process: %s with pid: %d", 
		current->comm, current->pid);
	// wait for complete
	complete(&mycomple);
	return count;
}
static int comple_open(struct inode *inodp, struct file *filp)
{
	return 0;
}
static int comple_release(struct inode *inodp, struct file *filp)
{
	return 0;
}

static int __init myinit(void)
{
	int retval;
	retval = alloc_chrdev_region(&mydev, 0, 1, "comple");
	if(retval < 0)
	{
		retval = -ENOMEM;
		printk(KERN_ALERT "Error to alloc major and minor number\n");
		goto out;
	}

	cdev_init(&cdev, &fops);
	cdev.owner = THIS_MODULE;
	cdev.ops = &fops;
	if(cdev_add(&cdev, mydev, 1))
	{
		retval = -ENOMEM;
		unregister_chrdev_region(mydev, 1);
		printk(KERN_ALERT "Error to add cdev to major and minor number\n");
		goto out;
	}
	return retval;
	out:
		return retval;
}

static void __exit myexit(void)
{
	cdev_del(&cdev);
	unregister_chrdev_region(mydev, 1);
}

module_init(myinit);
module_exit(myexit);