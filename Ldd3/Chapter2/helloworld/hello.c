#include<linux/init.h>
#include<linux/module.h>
#include<linux/sched.h>
#include<asm/current.h>
#include<linux/version.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("lol");
MODULE_AUTHOR("VinhLV5");
MODULE_VERSION("0.0.1");

int __initdata majorversion;
int __initdata minorversion;
int __initdata patchversion;

int symbolhello1 = 10;
int symbolhello2 = 20;

char *mystring = "Hello";

module_param(mystring, charp, S_IRUGO);

EXPORT_SYMBOL(symbolhello1);
EXPORT_SYMBOL_GPL(symbolhello2);

static int __init hello_init(void)
{
	printk(KERN_ALERT "Hello world\n");
	printk(KERN_ALERT "Current process is %s, Pid: %d\n", current->comm, current->pid);
	majorversion = ((unsigned int)LINUX_VERSION_CODE>>16)&0x000000ff;
	minorversion = ((unsigned int)LINUX_VERSION_CODE>>8)&0x000000ff;
	patchversion = (unsigned int)LINUX_VERSION_CODE&0x000000ff;
	printk(KERN_ALERT "Linux version code: %d.%d.%d\n", majorversion, minorversion, patchversion);
	printk(KERN_ALERT "Mystring = %s\n", mystring);
	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_ALERT "Goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);