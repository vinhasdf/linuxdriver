#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinh");

#define PEN_VENDOR	0x1908
#define PEN_DEVID	0x0226

#define MIN(arg1, arg2)		((arg1 < arg2)? arg1 : arg2)
#define USB_EP_01		0x01 		// OUT enpoint
#define USB_EP_02		0x81 		// IN enpoint
#define MAX_PKT_SIZE	512

static unsigned char pen_buff[MAX_PKT_SIZE];

struct usb_device *pen_device;
struct usb_class_driver class;

struct usb_device_id pend_ids[] = {
	{ USB_DEVICE(PEN_VENDOR, PEN_DEVID) },
	{}
};

MODULE_DEVICE_TABLE(usb, pend_ids);

static ssize_t pen_read(struct file *filp, char __user *buff, size_t count, loff_t *loff)
{
	int ret, actual;
	count = MIN(count, MAX_PKT_SIZE);

	ret = usb_bulk_msg(pen_device, usb_rcvbulkpipe(pen_device, USB_EP_02), pen_buff, MAX_PKT_SIZE, &actual, 1000);
	if(ret)
	{
		printk(KERN_INFO "Cannot read from usb device\n");
		return -EFAULT;
	}

	if(copy_to_user(buff, pen_buff, MIN(count, actual)))
	{
		printk(KERN_INFO "Cannot copy to user\n");
		return -EFAULT;
	}

	return MIN(count, actual);
}

static ssize_t pen_write(struct file *filp, const char __user *buff, size_t count, loff_t *loff)
{
	int ret, actual;
	count = MIN(count, MAX_PKT_SIZE);

	if(copy_from_user(pen_buff, buff, count))
	{
		printk(KERN_INFO "Cannot copy from user\n");
		return -EFAULT;
	}

	ret = usb_bulk_msg(pen_device, usb_sndbulkpipe(pen_device, USB_EP_01), pen_buff, count, &actual, 1000);
	if(ret)
	{
		printk(KERN_INFO "Cannot write to usb device\n");
		return -EFAULT;
	}

	return MIN(count, actual);
}

static int pen_open(struct inode *inodp, struct file *filp)
{
	return 0;
}

static int pen_close(struct inode *inodp, struct file *filp)
{
	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = pen_open,
	.release = pen_close,
	.read = pen_read,
	.write = pen_write,
};

int pend_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	int i;
	struct usb_host_interface *id_if;
	struct usb_endpoint_descriptor *endpoint;

	id_if = intf->cur_altsetting;

	printk(KERN_INFO "Pen i/f %d now probed: (%04X:%04X)\n", 
		id_if->desc.bInterfaceNumber, id->idVendor, id->idProduct);
	printk(KERN_INFO "Number of endpoint: %d\n", id_if->desc.bNumEndpoints);
	printk(KERN_INFO "Interface class: %02X\n", id_if->desc.bInterfaceClass);

	for(i = 0; i < id_if->desc.bNumEndpoints; i++)
	{
		endpoint = &id_if->endpoint[i].desc;
		printk(KERN_INFO "ED[%d] Address: %02X\n", i, endpoint->bEndpointAddress);
		printk(KERN_INFO "ED[%d] Attribute: %02X\n", i, endpoint->bmAttributes);
		printk(KERN_INFO "ED[%d] Max packet size: %d\n", i, endpoint->wMaxPacketSize);
	}

	pen_device = interface_to_usbdev(intf);
	class.name = "pen%d";
	class.fops = &fops;
	if(usb_register_dev(intf, &class) < 0)
	{
		printk(KERN_INFO "Not able to get a minor for this device\n");
		return -1;
	}
	else
	{
		printk(KERN_INFO "Minor obtained: %d\n", intf->minor);
	}

	return 0;
}

void pend_dis(struct usb_interface *intf)
{
	printk(KERN_INFO "I/f %d is disconnected\n", 
		intf->cur_altsetting->desc.bInterfaceNumber);
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
	printk(KERN_INFO "Good bye pend drivers\n");
}

module_init(pen_init);
module_exit(pen_exit);