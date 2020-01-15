#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/gpio.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinh");

#define PHYSICAL_BASE 	0x3f000000
#define OUTPUT_PIN		21
#define INPUT_PIN		7
#define GPIO_BASE_ADD	(PHYSICAL_BASE + 0x00200000)

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
			iowrite32(0x00000010, (void *)&gpio_base->GPPUD);
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

irqreturn_t hand_irq(int irqnum, void *arg)
{
	printk(KERN_INFO "Irq handled\n");
	return IRQ_HANDLED;
}

static int blink_init(void)
{
	gpio_base = ioremap(GPIO_BASE_ADD, 1024);
	if(gpio_base == NULL)
	{
		printk(KERN_INFO "Cannot remap io\n");
		return -ENOMEM;
	}
	pin_Mode(OUTPUT_PIN, OUTPUT);
	pin_Mode(INPUT_PIN, INPUT_PULLUP);
	pin_Output(OUTPUT_PIN, HIGH);
	
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

	if(request_irq(pin_irq, hand_irq, IRQF_SHARED, "blink", NULL) < 0)
	{
		printk(KERN_INFO "Cannot request irq");
		pin_Output(OUTPUT_PIN, LOW);
		iounmap(gpio_base);
		return -EFAULT;
	}

	printk(KERN_INFO "Init successfully\n");
	return 0;
}

static void blink_exit(void)
{
	free_irq(pin_irq, NULL);
	pin_Output(OUTPUT_PIN, LOW);
	if(pin_Input(INPUT_PIN))
	{
		printk(KERN_INFO "High\n");
	}
	else
	{
		printk(KERN_INFO "Low\n");
	}
	iounmap(gpio_base);
	printk(KERN_INFO "Goodbye\n");
}

module_init(blink_init);
module_exit(blink_exit);