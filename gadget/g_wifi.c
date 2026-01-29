#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/module.h>
#include <linux/sched.h>

static int g_wifi_composite_bind(struct usb_composite_dev *cdev);
static int g_wifi_composite_unbind(struct usb_composite_dev *cdev);

struct gwifi
{
    struct usb_function func;

    /* Endpoints. */
    struct usb_ep *ep_in;
    struct usb_ep *ep_out;

    /* Completion callbacks. */
    struct usb_request *req_in;
    struct usb_request *req_out;
};

struct remote_wiphy_dev
{
    struct gwifi *port_usb;
};

/* String descriptors. */
static struct usb_string strings_dev[] = {
    [USB_GADGET_MANUFACTURER_IDX].s = REMOTE_WIPHY_MANUFACTURER,
    [USB_GADGET_PRODUCT_IDX].s = REMOTE_WIPHY_PRODUCT,
    [USB_GADGET_SERIAL_IDX].s = REMOTE_WIPHY_SERIAL,
    {}};

static struct usb_gadget_strings stringtab_dev = {
    .language = 0x0409, /* en-us */
    .strings = strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
    &stringtab_dev,
    NULL,
};

/* Device descriptor. */
static struct usb_device_descriptor device_desc = {
    .bLength = sizeof(device_desc),
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = cpu_to_le16(0x0200), /* USB 2.0 */
    .bDeviceClass = USB_CLASS_VENDOR_SPEC,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = cpu_to_le16(REMOTE_WIPHY_VENDOR_ID),
    .idProduct = cpu_to_le16(REMOTE_WIPHY_PRODUCT_ID),
    .bcdDevice = cpu_to_le16(REMOTE_WIPHY_DEVICE_VERSION),
    .iManufacturer = 0,
    .iProduct = 0,
    .iSerialNumber = 0,
    .bNumConfigurations = 1,
};

/* Interface descriptor. */
static struct usb_interface_descriptor intf_desc = {
    .bLength = sizeof(intf_desc),
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 3,
    .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 0,
};

/* Endpoint descriptors. */
static struct usb_endpoint_descriptor ep_out_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_OUT | 1,    /* Host -> Dev, ep 1. */
    .bmAttributes = USB_ENDPOINT_XFER_BULK, /* Transfer bulk data. */
    .wMaxPacketSize = cpu_to_le16(64),
};

static struct usb_endpoint_descriptor ep_in_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN | 2,     /* Dev -> Host, ep 2. */
    .bmAttributes = USB_ENDPOINT_XFER_BULK, /* Transfer bulk data. */
    .wMaxPacketSize = cpu_to_le16(64),
};

static struct usb_endpoint_descriptor ep_int_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN | 3,    /* Dev -> Host - ep 3. */
    .bmAttributes = USB_ENDPOINT_XFER_INT, /* Interrupt. */
    .wMaxPacketSize = cpu_to_le16(64),
};

static struct usb_composite_driver wifi_driver = {
    .name = "WiFi Adapter",
    .dev = &device_desc,
    .strings = dev_strings,
    .max_speed = USB_SPEED_SUPER,
    .bind = g_wifi_composite_bind,     /* USB is plugged in. */
    .unbind = g_wifi_composite_unbind, /* USB is unplugged. */
};

/**
 * @brief   - The callback is called
 */
static int g_wifi_composite_bind(struct usb_composite_dev *cdev)
{
    int ret = 0;
    struct usb_configuration *configuration;
    struct usb_function *func;
    struct usb_string *s;

    /* 1. Set string descriptors. */
	ret = usb_string_ids_tab(cdev, strings_dev);
	if (ret < 0)
    {
        pr_error("Failed to get string id table: %d\n", ret);
        goto fail;
    }

    device_desc.iManufacturer = strings_dev[USB_GADGET_MANUFACTURER_IDX].id;
    device_desc.iProduct = strings_dev[USB_GADGET_PRODUCT_IDX].id;
    device_desc.iSerialNumber = strings_dev[USB_GADGET_SERIAL_IDX].id;

    /* 2. Set configuration. */

fail:
    return ret;
}

static int g_wifi_composite_unbind(struct usb_composite_dev *cdev)
{
    int ret = 0;
    pr_info("Unbind\n");

    return ret;
}

module_usb_composite_driver(wifi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeddedOS");
MODULE_DESCRIPTION("I'm a WiFi adapter!");