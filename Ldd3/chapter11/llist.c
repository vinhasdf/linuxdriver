#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinh");

struct todo_struct
{
	u8 priority;
	char buff[100];
	struct list_head mlist;	
};

struct cdev cdev;
dev_t llist_dev;
LIST_HEAD(todo_list);
struct class *llist_class;

static ssize_t llist_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t llist_write(struct file *, const char __user *, size_t, 
	loff_t *);
static int llist_open(struct inode *, struct file *);
static int llist_release(struct inode *, struct file *);

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = llist_open,
	.release = llist_release,
	.read = llist_read,
	.write = llist_write,
};

static ssize_t llist_read(struct file *filp, char __user *buff, size_t count, loff_t *fpos)
{

	struct list_head *ptr;
	struct todo_struct *dat_read;

	ptr = todo_list.next;
	if(ptr != NULL && ptr != &todo_list)
	{
		dat_read = list_entry(ptr, struct todo_struct, mlist);
		if(count > 100)
			count = 100;
		if(copy_to_user(buff, dat_read->buff, count))
		{
			return -EFAULT;
		}
		list_del(ptr);
		kfree(dat_read);
	}
	else
	{
		count = 0;
	}
	return count;
}

static ssize_t llist_write(struct file *filp, const char __user *buff, size_t count, loff_t *fpos)
{
	struct todo_struct *dat_write = NULL;
	struct todo_struct *entry;
	struct list_head *ptr;

	if(count > 0)
	{
		dat_write = kmalloc(sizeof(struct todo_struct), GFP_KERNEL);
		if(dat_write != NULL)
		{
			memset(dat_write->buff, 0, 100);
			if(count > 100)
				count = 100;
			if(copy_from_user(dat_write->buff, buff, count))
			{
				kfree(dat_write);
				return -EFAULT;
			}
			dat_write->priority = (u8)dat_write->buff[0];
			for(ptr = todo_list.next; ptr != &todo_list; ptr = ptr->next)
			{
				entry = list_entry(ptr, struct todo_struct, mlist);
				if(dat_write->priority > entry->priority)
				{
					list_add_tail(&dat_write->mlist, ptr);
					return count;
				}
			}
			list_add_tail(&dat_write->mlist, &todo_list);
		}
		else
		{
			return -ENOMEM;
		}
	}

	return count;
}

static int llist_open(struct inode *inodp, struct file *filp)
{
	return 0;
}

static int llist_release(struct inode *inodp, struct file *filp)
{
	return 0;
}

static int llist_init(void)
{
	int ret;
	ret = alloc_chrdev_region(&llist_dev, 0, 1, "llist");
	if(ret < 0)
	{
		printk(KERN_INFO "Cannot alloc major num\n");
		return -ENOMEM;
	}

	cdev_init(&cdev, &fops);
	cdev.owner = THIS_MODULE;
	cdev.ops = &fops;
	ret = cdev_add(&cdev, llist_dev, 1);
	if(ret)
	{
		printk(KERN_INFO "Cannot assign cdev to device\n");
		ret = -ENOMEM;
		goto out;
	}

	llist_class = class_create(THIS_MODULE, "llist_class");
	if(llist_class == NULL)
	{
		printk(KERN_INFO "Cannot alloc class\n");
		ret = -ENOMEM;
		goto out1;	
	}

	device_create(llist_class, NULL, llist_dev, NULL, "llist");

	printk(KERN_INFO "Init succesfully\n");

	return 0;

	out1:
	cdev_del(&cdev);
	out:
	unregister_chrdev_region(llist_dev, 1);
	return ret;
}

static void llist_exit(void)
{
	struct todo_struct *entry;
	struct list_head *ptr;

	for(ptr = todo_list.next; ptr != &todo_list; ptr = ptr->next)
	{
		entry = list_entry(ptr, struct todo_struct, mlist);
		list_del(ptr);
		kfree(entry);
	}

	device_destroy(llist_class, llist_dev);
	class_destroy(llist_class);
	cdev_del(&cdev);
	unregister_chrdev_region(llist_dev, 1);
	printk(KERN_INFO "Goodbye\n");
}

module_init(llist_init);
module_exit(llist_exit);