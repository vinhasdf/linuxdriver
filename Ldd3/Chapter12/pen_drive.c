#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinh");

#define PEN_VENDOR	0x1908
#define PEN_DEVID	0x0226

struct usb_device_id pend_ids[] = {
	{ USB_DEVICE(PEN_VENDOR, PEN_DEVID) },
	{}
};

MODULE_DEVICE_TABLE(usb, pend_ids);

int pend_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	printk(KERN_INFO "Pen drive (%04X:%04X) plugged\n", id->idVendor, id->idProduct);
	return 0;
}

void pend_dis(struct usb_interface *intf)
{
	printk(KERN_INFO "Pen drive removed\n");
}

struct usb_driver pen_fops = {
	.name = "pen_drive",
	.id_table = pend_ids,
	.probe = pend_probe,
	.disconnect = pend_dis,
};

static int pen_init(void)
{
	int ret;
	ret = usb_register(&pen_fops);
	if(ret)
		printk(KERN_INFO "Usb cannot register to kernel\n");
	return ret;
}

static void pen_exit(void)
{
	usb_deregister(&pen_fops);
	printk(KERN_INFO "Good bye pend drivers");
}

module_init(pen_init);
module_exit(pen_exit);