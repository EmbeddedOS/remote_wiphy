#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

#include "../usb_descriptors.h"

static int
wiphy_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void wiphy_disconnect(struct usb_interface *interface);

static const struct usb_device_id id_table[] = {
    { USB_DEVICE(REMOTE_WIPHY_VENDOR_ID, REMOTE_WIPHY_PRODUCT_ID) },
    { }
};

MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver remote_wiphy_driver = {
    .name       = "remote_wifi_driver",
    .probe      = wiphy_probe,
    .disconnect = wiphy_disconnect,
    .id_table   = id_table,
};

static int wiphy_probe(struct usb_interface *interface,
                       const struct usb_device_id *id)
{ // Called when device is plugged in.
    return 0;
}

static void wiphy_disconnect(struct usb_interface *interface)
{ // Called when device is unplugged.
}

module_usb_driver(remote_wiphy_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeddedOS");
MODULE_DESCRIPTION("Remote WiPhy driver");