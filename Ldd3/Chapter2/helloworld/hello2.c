#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("lol");
MODULE_AUTHOR("VinhLV5");
MODULE_VERSION("0.0.1");

int __exitdata myexitdata = 1;

extern int symbolhello1;
extern int symbolhello2;

static int __init myinit(void)
{
	printk(KERN_ALERT "Module hello2, Sym1 = %d, Sym2 = %d\n", symbolhello1, symbolhello2);
	return 0;
}

static void __exit myexit(void)
{
	printk(KERN_ALERT "Unload module hello2, exit data: %d\n", myexitdata);
}

module_init(myinit);
module_exit(myexit);