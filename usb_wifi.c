#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/module.h>
#include <linux/sched.h>

static int wifi_bind(struct usb_composite_dev *cdev);
static int wifi_unbind(struct usb_composite_dev *cdev);

static struct usb_device_descriptor device_desc = {
    .bLength            = sizeof(device_desc),
};

/* Human readable descriptions that will exposed to the host. */
static struct usb_string strings_dev[] = {
    [USB_GADGET_MANUFACTURER_IDX].s = "",
    [USB_GADGET_PRODUCT_IDX].s = "WiFi Adapter",
    [USB_GADGET_SERIAL_IDX].s = "",
    {  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
    .language = 0x0409,  /* en-us */
    .strings  = strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
    &stringtab_dev,
    NULL,
};

static struct usb_composite_driver wifi_driver = {
    .name       = "WiFi Adapter",
    .dev        = &device_desc,
    .strings    = dev_strings,
    .max_speed  = USB_SPEED_SUPER,
    .bind       = wifi_bind,
    .unbind     = wifi_unbind,
};

static int wifi_bind(struct usb_composite_dev *cdev)
{ // Called when the USB cable is connected.
    int ret = 0;
    pr_info("Bind\n");

    return ret;
}

static int wifi_unbind(struct usb_composite_dev *cdev)
{ // Called when the USB cable is disconnected.
    int ret = 0;
    pr_info("Unbind\n");

    return ret;
}


module_usb_composite_driver(wifi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeddedOS");
MODULE_DESCRIPTION("I'm a WiFi adapter!");