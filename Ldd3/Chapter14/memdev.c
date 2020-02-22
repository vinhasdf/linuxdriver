#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinh");

dev_t memdev_devicenum;
char *membuff;
struct cdev cdev;
struct class *memclass;

static int mem_open(struct inode *inodp, struct file *filp)
{
    return 0;
}

static int mem_close(struct inode *inodp, struct file *filp)
{
    return 0;
}

static ssize_t mem_read(struct file *filp, char __user *buff, size_t count, loff_t * loff)
{
    count = ((count > PAGE_SIZE)? PAGE_SIZE : count);
    if(copy_to_user(buff, membuff, count))
    {
        printk(KERN_INFO "Cannot copy data to user\n");
        return -EFAULT;
    }

    return count;
}

static ssize_t mem_write(struct file *filp, const char __user *buff, size_t count, loff_t *loff)
{
    count = ((count > PAGE_SIZE)? PAGE_SIZE : count);
    memset(membuff, 0, PAGE_SIZE);
    if(copy_from_user(membuff, buff, count))
    {
        printk(KERN_INFO "Cannot copy data from user\n");
        return -EFAULT;
    }
    return count;
}

static int mem_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long map_size;
    unsigned long physic;

    map_size = vma->vm_end - vma->vm_start;

    if(map_size > PAGE_SIZE)
    {
        printk(KERN_INFO "Invalid size\n");
        return -EINVAL;
    }

    physic = virt_to_phys(membuff + offset);
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP); // == VM_RESERVED at old kernel version

    if(remap_pfn_range(vma, vma->vm_start, physic >> PAGE_SHIFT, map_size, vma->vm_page_prot))
    {
        printk(KERN_INFO "Cannot mapping to user space\n");
        return -EAGAIN;
    }

    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = mem_open,
    .release = mem_close,
    .read = mem_read,
    .write = mem_write,
    .mmap = mem_mmap,
};

static int memdev_init(void)
{
    int ret;

    membuff = __get_free_page(GFP_KERNEL);

    if(alloc_chrdev_region(&memdev_devicenum, 0, 1, "memdev"))
    {
        printk(KERN_INFO "Cannot alloc device number\n");
        ret = -ENOMEM;
        goto faildevnum;
    }

    memclass = class_create(THIS_MODULE, "memclass");
    if(memclass == NULL)
    {
        printk(KERN_INFO "Cannot alloc mem class\n");
        ret = -ENOMEM;
        goto failclass;
    }
    device_create(memclass, NULL, memdev_devicenum, NULL, "memdev");

    cdev_init(&cdev, &fops);
    ret = cdev_add(&cdev, memdev_devicenum, 1);
    if(ret < 0)
    {
        printk(KERN_INFO "Cannot add cdev\n");
        goto cdevfail;
    }

    printk(KERN_INFO "Mem device is allocate successfully\n");

    return 0;
    cdevfail:
    device_destroy(memclass, memdev_devicenum);
    class_destroy(memclass);
    failclass:
    unregister_chrdev_region(memdev_devicenum, 1);
    faildevnum:
    free_page(membuff);
    return ret;
}

static void memdev_exit(void)
{
    cdev_del(&cdev);
    device_destroy(memclass, memdev_devicenum);
    class_destroy(memclass);
    unregister_chrdev_region(memdev_devicenum, 1);
    free_page(membuff);
}

module_init(memdev_init);
module_exit(memdev_exit);