#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/module.h>
#include <linux/sched.h>

static int g_wifi_bind(struct usb_composite_dev *cdev);
static int g_wifi_unbind(struct usb_composite_dev *cdev);

static struct usb_device_descriptor device_desc = {
    .bLength            = sizeof(device_desc),
    .bDescriptorType    = USB_DT_DEVICE,
    .bcdUSB             = cpu_to_le16(0x0200),  /* USB 2.0 */
    .bDeviceClass       = USB_CLASS_VENDOR_SPEC,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 64,
    .idVendor           = cpu_to_le16(REMOTE_WIPHY_VENDOR_ID),
    .idProduct          = cpu_to_le16(REMOTE_WIPHY_PRODUCT_ID),
    .bcdDevice          = cpu_to_le16(REMOTE_WIPHY_DEVICE_VERSION),
    .iManufacturer      = 0,
    .iProduct           = 0,
    .iSerialNumber      = 0,
    .bNumConfigurations = 1,
};

/* Human readable descriptions that will exposed to the host. */
static struct usb_string strings_dev[] = {
    [USB_GADGET_MANUFACTURER_IDX].s = "EmbeddedOS",
    [USB_GADGET_PRODUCT_IDX].s = "An USB Gadget WiFi Adapter",
    [USB_GADGET_SERIAL_IDX].s = "",
    {  }
};


/* Configuration descriptors. */

/* Interface descriptors. */


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
    .bind       = g_wifi_bind,
    .unbind     = g_wifi_unbind,
};

static int g_wifi_bind(struct usb_composite_dev *cdev)
{ // Called when the USB cable is connected.
    int ret = 0;
    pr_info("Bind\n");

    return ret;
}

static int g_wifi_unbind(struct usb_composite_dev *cdev)
{ // Called when the USB cable is disconnected.
    int ret = 0;
    pr_info("Unbind\n");

    return ret;
}


module_usb_composite_driver(wifi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeddedOS");
MODULE_DESCRIPTION("I'm a WiFi adapter!");