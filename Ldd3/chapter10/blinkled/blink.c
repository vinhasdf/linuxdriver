#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/jiffies.h>
#include <linux/wait.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinh");

#define PHYSICAL_BASE 	0x3f000000
#define INPUT_PIN		7
#define GPIO_BASE_ADD	(PHYSICAL_BASE + 0x00200000)
#define BUFFER_SIZE		5
#if(BUFFER_SIZE == 0)
#error "BUFFER_SIZE cannot zero"
#endif
#define TIME_OUT		3

typedef struct {
	unsigned int GPFSEL[6];
	unsigned int reserved0;
	unsigned int GPSET[2];
	unsigned int reserved1;
	unsigned int GPCLR[2];
	unsigned int reserved2;
	unsigned int GPLEV[2];
	unsigned int reserved3;
	unsigned int GPEDS[2];
	unsigned int reserved4;
	unsigned int GPREN[2];
	unsigned int reserved5;
	unsigned int GPFEN[2];
	unsigned int reserved6;
	unsigned int GPHEN[2];
	unsigned int reserved7;
	unsigned int GPLEN[2];
	unsigned int reserved8;
	unsigned int GPAREN[2];
	unsigned int reserved9;
	unsigned int GPAFEN[2];
	unsigned int reserved10;
	unsigned int GPPUD;
	unsigned int GPPUDCLK[2];
} Gpio_type;

typedef enum{
	INPUT = 0,
	OUTPUT,
	INPUT_PULLUP,
	INPUT_PULLDOWN
} pin_Direction;

typedef enum{
	LOW = 0,
	HIGH
} pin_Level;

Gpio_type *gpio_base;
unsigned int pin_irq;
int Output_Pins[4] = {21, 20, 16, 12}; // msb to lsb
// struct workqueue_struct *myworkqueue;
// struct work_struct mywork;
int state = 0;
unsigned char Blink_Queue[BUFFER_SIZE] = {0};
int current_read = 0, current_write = 0, queue_size = 0;
dev_t blink_dev;
struct cdev cdev;
struct class *blink_class;
spinlock_t blink_lock;
//DECLARE_COMPLETION(blink_comple);
unsigned long oldjif = 0;
DECLARE_WAIT_QUEUE_HEAD(blink_wait);

int enqueue_val(unsigned char val)
{
	if(queue_size < BUFFER_SIZE)
	{
		queue_size++;
		Blink_Queue[current_write++] = val;
		if(current_write == BUFFER_SIZE)
			current_write = 0;
		return 0;
	}
	else
	{
		return -1;
	}
}

int dequeue_val(unsigned char *ret)
{
	if(queue_size > 0)
	{
		queue_size--;
		*ret = Blink_Queue[current_read++];
		if(current_read == BUFFER_SIZE)
			current_read = 0;
		return 0;
	}
	else
	{
		return -1;
	}
}

void pin_Output(int pin, pin_Level lev)
{
	int num, offset;
	num = pin/32;
	offset = pin%32;
	if(lev == HIGH)
		iowrite32(((unsigned int)1 << offset), (void *)&gpio_base->GPSET[num]);
	else
		iowrite32(((unsigned int)1 << offset), (void *)&gpio_base->GPCLR[num]);
}

static ssize_t blink_write(struct file *filp, const char __user *buff, size_t count, 
	loff_t *fpos)
{
	unsigned long flags;
	unsigned char val_enqueue, a;
	int retqueue, i;
	long ret;
	if(count < 1)
	{
		return -EFAULT;
	}
	count = 1;

	if(copy_from_user(&val_enqueue, buff, count))
	{
		return -EFAULT;
	}

	spin_lock_irqsave(&blink_lock, flags);
	retqueue = enqueue_val(val_enqueue);
	while(retqueue == -1)
	{
		spin_unlock_irqrestore(&blink_lock, flags);
		ret = wait_event_interruptible_timeout(blink_wait, queue_size != BUFFER_SIZE, TIME_OUT*HZ);
		if(ret < 0)
		{
			return -ERESTARTSYS;
		}
		spin_lock_irqsave(&blink_lock, flags);
		if(ret == 0) // case timeout
		{
			if(dequeue_val(&a) == 0)
			{
				for(i = 0; i < 4; i++)
				{
					if(a & ((unsigned char)0x08 >> i))
					{
						pin_Output(Output_Pins[i], HIGH);
					}
					else
					{
						pin_Output(Output_Pins[i], LOW);
					}
				}
			}
		}
		retqueue = enqueue_val(val_enqueue);
	}
	spin_unlock_irqrestore(&blink_lock, flags);
	return count;
}

pin_Level pin_Input(int pin)
{
	int num, offset;
	unsigned int temp;
	num = pin/32;
	offset = pin%32;
	temp = ioread32((void *)&gpio_base->GPLEV[num]);
	if(temp & ((unsigned int)1 << offset))
		return HIGH;
	else
		return LOW;
}

static ssize_t blink_read(struct file *filp, char __user *buff, size_t count, loff_t *fpos)
{
	int i;
	char retval = 0;
	for(i = 0; i < 4; i++)
	{
		if(pin_Input(Output_Pins[i]) == HIGH)
		{
			retval |= ((unsigned char)0x08 >> i);
		}
		else
		{
		}
	}
	if(count > 1)
		count = 1;
	if(copy_to_user(buff, &retval, count))
	{
		return -EFAULT;
	}
	return count;
}

static int blink_open(struct inode *inodp, struct file *filp)
{
	return 0;
}

static int blink_close(struct inode *inodp, struct file *filp)
{
	return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = blink_read,
	.write = blink_write,
	.open = blink_open,
	.release = blink_close,
};



void pin_Mode(int pin, pin_Direction dir)
{
	int num, offset;
	unsigned int temp;
	num = pin/10;
	offset = (pin%10) * 3;
	temp = ioread32((void *)&gpio_base->GPFSEL[num]);
	if(dir == OUTPUT)
		iowrite32(temp|((unsigned int)1 << offset), (void *)&gpio_base->GPFSEL[num]);
	else
	{
		iowrite32(temp & (~((unsigned int)7 << offset)), (void *)&gpio_base->GPFSEL[num]);
		offset /= 3;
		if(dir == INPUT_PULLUP)
		{
			iowrite32(0x00000002, (void *)&gpio_base->GPPUD);
			udelay(1);
			temp = ioread32(&gpio_base->GPPUDCLK[num]);
			iowrite32(temp|((unsigned int)1 << offset), (void *)&gpio_base->GPPUDCLK[num]);
			udelay(1);
			iowrite32(0, (void *)&gpio_base->GPPUD);
			iowrite32(temp & (~((unsigned int)1 << offset)), (void *)&gpio_base->GPPUDCLK[num]);

			temp = ioread32((void *)&gpio_base->GPFEN[num]);
			iowrite32(temp | ((unsigned int)1 << offset), (void *)&gpio_base->GPFEN[num]);

		}
		else if(dir == INPUT_PULLDOWN)
		{
			iowrite32(0x00000001, (void *)&gpio_base->GPPUD);
			udelay(1);
			temp = ioread32(&gpio_base->GPPUDCLK[num]);
			iowrite32(temp|((unsigned int)1 << offset), (void *)&gpio_base->GPPUDCLK[num]);
			udelay(1);
			iowrite32(0, (void *)&gpio_base->GPPUD);
			iowrite32(temp & (~((unsigned int)1 << offset)), (void *)&gpio_base->GPPUDCLK[num]);

			temp = ioread32((void *)&gpio_base->GPREN[num]);
			iowrite32(temp | ((unsigned int)1 << offset), (void *)&gpio_base->GPREN[num]);

		}
	}
}

irqreturn_t hand_irq(int irqnum, void *arg)
{
	int num, offset, i;
	unsigned char a;
	num = INPUT_PIN/32;
	offset = INPUT_PIN%32;
	iowrite32(((unsigned int)1<<offset), (void *)&gpio_base->GPEDS[num]);
	//queue_work(myworkqueue, &mywork);

	if((unsigned long)(jiffies - oldjif) >= HZ)
	{
		oldjif = jiffies;
		spin_lock(&blink_lock);
		if(dequeue_val(&a) == 0)
		{
			for(i = 0; i < 4; i++)
			{
				if(a & ((unsigned char)0x08 >> i))
				{
					pin_Output(Output_Pins[i], HIGH);
				}
				else
				{
					pin_Output(Output_Pins[i], LOW);
				}
			}
			spin_unlock(&blink_lock);
		}
		else
			spin_unlock(&blink_lock);

		wake_up_interruptible(&blink_wait);
		printk(KERN_INFO "Irq handled %d\n", a);
	}
	return IRQ_HANDLED;
}

// void callback_work(struct work_struct *arg)
// {
// 	int i;
// 	for(i = 0; i < 20; i++)
// 	{
// 		mdelay(1);
// 		if(pin_Input(INPUT_PIN) == HIGH)
// 		{
// 			state = 0;
// 			break;
// 		}
// 	}
// 	if((i == 20) && (state == 0))
// 	{
// 		state = 1;
// 		printk(KERN_INFO "Irq handled\n");
// 		pin_Output(OUTPUT_PIN, !pin_Input(OUTPUT_PIN));
// 	}
// }

static int blink_init(void)
{
	int i;
	int ret;

	ret = alloc_chrdev_region(&blink_dev, 0, 1, "blink");
	if(ret < 0)
	{
		printk(KERN_INFO "Cannot alloc major num\n");
		return -ENOMEM;
	}

	cdev_init(&cdev, &fops);
	cdev.owner = THIS_MODULE;
	cdev.ops = &fops;
	ret = cdev_add(&cdev, blink_dev, 1);
	if(ret)
	{
		printk(KERN_INFO "Cannot assign cdev to device\n");
		ret = -ENOMEM;
		goto out;
	}

	blink_class = class_create(THIS_MODULE, "blink_class");
	if(blink_class == NULL)
	{
		printk(KERN_INFO "Cannot alloc class\n");
		ret = -ENOMEM;
		goto out1;	
	}

	device_create(blink_class, NULL, blink_dev, NULL, "blink");

	gpio_base = ioremap(GPIO_BASE_ADD, 1024);
	if(gpio_base == NULL)
	{
		printk(KERN_INFO "Cannot remap io\n");
		ret = -ENOMEM;
		goto out2;
	}
	pin_Mode(INPUT_PIN, INPUT_PULLUP);
	//pin_Output(OUTPUT_PIN, HIGH);
	for(i = 0; i < 4; i++)
	{
		pin_Mode(Output_Pins[i], OUTPUT);
		pin_Output(Output_Pins[i], HIGH);
	}
	
	if(pin_Input(INPUT_PIN))
	{
		printk(KERN_INFO "High\n");
	}
	else
	{
		printk(KERN_INFO "Low\n");
	}

	pin_irq = gpio_to_irq(INPUT_PIN);

	printk(KERN_INFO "Pin irq: %d\n", pin_irq);

	// myworkqueue = create_workqueue("myworkqueue");
	// INIT_WORK(&mywork, callback_work);

	if(request_irq(pin_irq, hand_irq, IRQF_SHARED, "blink", (void *)&blink_dev) < 0)
	{
		printk(KERN_INFO "Cannot request irq");
		for(i = 0; i < 4; i++)
		{
			pin_Output(Output_Pins[i], LOW);
		}
		iounmap(gpio_base);
		ret = -EFAULT;
		goto out2;
	}

	spin_lock_init(&blink_lock);

	printk(KERN_INFO "Init successfully\n");
	return 0;


	out2:
	device_destroy(blink_class, blink_dev);
	class_destroy(blink_class);
	out1:
	cdev_del(&cdev);
	out:
	unregister_chrdev_region(blink_dev, 1);
	return ret;
}

static void blink_exit(void)
{
	int i;
	free_irq(pin_irq, &blink_dev);
	for(i = 0; i < 4; i++)
	{
		pin_Output(Output_Pins[i], LOW);
	}
	if(pin_Input(INPUT_PIN))
	{
		printk(KERN_INFO "High\n");
	}
	else
	{
		printk(KERN_INFO "Low\n");
	}
	iounmap(gpio_base);
	device_destroy(blink_class, blink_dev);
	class_destroy(blink_class);
	cdev_del(&cdev);
	unregister_chrdev_region(blink_dev, 1);
	printk(KERN_INFO "Goodbye\n");
}

module_init(blink_init);
module_exit(blink_exit);