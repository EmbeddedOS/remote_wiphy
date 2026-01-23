#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/module.h>
#include <linux/sched.h>


static struct usb_device_descriptor device_desc = {
    .bLength            = sizeof(device_desc),
};

static int __init usb_wifi_init(void)
{

    return 0;
}

static void __exit usb_wifi_exit(void)
{

}

module_init(usb_wifi_init);
module_exit(usb_wifi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeddedOS");
MODULE_DESCRIPTION("I'm a WiFi adapter!");