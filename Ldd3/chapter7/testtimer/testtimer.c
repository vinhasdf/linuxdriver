#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinh");

struct timer_list tim;
struct tasklet_struct ta;
struct workqueue_struct *myworkqueue;
struct work_struct mywork;


void task_func(unsigned long arg)
{
	printk(KERN_INFO "Delay 20ms\n");
	mdelay(20);
	printk(KERN_INFO "After delay\n");
}

void tim_func(struct timer_list *arg)
{
	printk("jiffies: %ld\n", jiffies);
}

void callback_work(struct work_struct *arg)
{
	printk(KERN_INFO "In work queue\n");
}

static int testtim_init(void)
{
	timer_setup(&tim, tim_func, 0);
	tim.expires = jiffies + HZ;
	add_timer(&tim);
	mdelay(1);

	del_timer_sync(&tim);
	tim.expires = jiffies + 2*HZ;
	add_timer(&tim); // segmenttation fault if add_timer two time, need del timer prior
	printk(KERN_INFO "HZ = %d, jiffies: %ld\n", HZ, jiffies);
	tasklet_init(&ta, task_func, 0);
	tasklet_schedule(&ta);

	myworkqueue = create_workqueue("myworkqueue");
	INIT_WORK(&mywork, callback_work);

	queue_work(myworkqueue, &mywork);
	queue_work(myworkqueue, &mywork);

	return 0;
}

static void testtim_exit(void)
{
	flush_workqueue(myworkqueue);
	destroy_workqueue(myworkqueue);
	tasklet_kill(&ta);
	del_timer_sync(&tim);
	printk(KERN_INFO "Goodbye\n");
}

module_init(testtim_init);
module_exit(testtim_exit);