#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vinh");

#define PEN_VENDOR	0x1908
#define PEN_DEVID	0x0226

struct usb_device *pen_device;

struct usb_device_id pend_ids[] = {
	{ USB_DEVICE(PEN_VENDOR, PEN_DEVID) },
	{}
};

MODULE_DEVICE_TABLE(usb, pend_ids);

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