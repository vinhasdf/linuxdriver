#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinhasdf");

#define SIMPLE_BLOCK_SECTOR_SIZE    512
#define SIMPLE_BLOCK_NUM_SECTOR     100
#define KERNEL_SECTOR_SIZE          512

struct gendisk *simpledisk;
int major;

static struct block_device_operations blfops = {
    .owner = THIS_MODULE,
};

static void bl_request(struct request_queue *q)
{

}

static int block_init(void)
{
    int ret;
    major = register_blkdev(0, "simple_block"); // Dynamic allocated
    if(major < 0)
    {
        printk(KERN_INFO "Cannot alloc major for simple block\n");
        return major;
    }
    simpledisk = alloc_disk(1);
    if(simpledisk == NULL)
    {
        printk(KERN_INFO "Cannot alloc gendisk struct\n");
        ret = -ENOMEM;
        goto out1;
    }

    simpledisk->major = major;
    simpledisk->first_minor = 0;
    simpledisk->minors = 1;
    simpledisk->fops = &blfops;
    strncpy(simpledisk->disk_name, "simple_block", sizeof(simpledisk->disk_name));
    set_capacity(simpledisk, SIMPLE_BLOCK_NUM_SECTOR * 
        (SIMPLE_BLOCK_SECTOR_SIZE/KERNEL_SECTOR_SIZE));
    simpledisk->queue = blk_init_queue(bl_request, NULL);
    if(simpledisk->queue == NULL)
    {
        ret = -ENOMEM;
        printk(KERN_INFO "Cannot alloc queue struct\n");
        goto out2;
    }

    add_disk(simpledisk);

    printk(KERN_INFO "Init simple block device successfully\n");

    return 0;
    out2:
    put_disk(simpledisk);
    out1:
    unregister_blkdev(major, "simple_block");
    return ret;
}

static void block_exit(void)
{
    blk_cleanup_queue(simpledisk->queue);
    del_gendisk(simpledisk);
    put_disk(simpledisk);
    unregister_blkdev(major, "simple_block");
    printk(KERN_INFO "Goodbye simple disk\n");
}

module_init(block_init);
module_exit(block_exit);