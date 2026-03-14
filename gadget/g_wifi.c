#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/module.h>
#include <linux/sched.h>

#include "../usb_descriptors.h"

static int gwifi_composite_bind(struct usb_composite_dev *cdev);
static int gwifi_composite_unbind(struct usb_composite_dev *cdev);

static int gwifi_bind(struct usb_configuration *config,
                      struct usb_function *func);

static void gwifi_unbind(struct usb_configuration *config,
                        struct usb_function *func);

static int gwifi_set_alt(struct usb_function *func,
                         unsigned interface, unsigned alt);

static void gwifi_disable(struct usb_function *func);

struct gwifi
{
    struct usb_function func;

    /* Endpoints. */
    struct usb_ep *ep_in;
    struct usb_ep *ep_out;
    struct usb_ep *ep_int;

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

/* Endpoint descriptors for Full Speed Mode: Max 64 bytes per packets. */
static struct usb_endpoint_descriptor ep_fs_out_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_OUT | 1,    /* Host -> Dev, ep 1. */
    .bmAttributes = USB_ENDPOINT_XFER_BULK, /* Transfer bulk data. */
    .wMaxPacketSize = cpu_to_le16(64),
};

static struct usb_endpoint_descriptor ep_fs_in_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN | 2,     /* Dev -> Host, ep 2. */
    .bmAttributes = USB_ENDPOINT_XFER_BULK, /* Transfer bulk data. */
    .wMaxPacketSize = cpu_to_le16(64),
};

static struct usb_endpoint_descriptor ep_fs_int_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN | 3,    /* Dev -> Host - ep 3. */
    .bmAttributes = USB_ENDPOINT_XFER_INT, /* Interrupt. */
    .wMaxPacketSize = cpu_to_le16(64),
};

static struct usb_descriptor_header *fs_func_desc[] = {
    (struct usb_descriptor_header *) &intf_desc,
    (struct usb_descriptor_header *) &ep_fs_out_desc,
    (struct usb_descriptor_header *) &ep_fs_in_desc,
    (struct usb_descriptor_header *) &ep_fs_int_desc,
    NULL,
};

/* Endpoint descriptors for High Speed Mode: Max 512 bytes per packets. */
static struct usb_endpoint_descriptor ep_hs_out_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_OUT | 1,    /* Host -> Dev, ep 1. */
    .bmAttributes = USB_ENDPOINT_XFER_BULK, /* Transfer bulk data. */
    .wMaxPacketSize = cpu_to_le16(512),
};

static struct usb_endpoint_descriptor ep_hs_in_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN | 2,     /* Dev -> Host, ep 2. */
    .bmAttributes = USB_ENDPOINT_XFER_BULK, /* Transfer bulk data. */
    .wMaxPacketSize = cpu_to_le16(512),
};

static struct usb_endpoint_descriptor ep_hs_int_desc = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN | 3,    /* Dev -> Host - ep 3. */
    .bmAttributes = USB_ENDPOINT_XFER_INT, /* Interrupt. */
    .wMaxPacketSize = cpu_to_le16(512),
};

static struct usb_descriptor_header *hs_func_desc[] = {
    (struct usb_descriptor_header *) &intf_desc,
    (struct usb_descriptor_header *) &ep_hs_out_desc,
    (struct usb_descriptor_header *) &ep_hs_in_desc,
    (struct usb_descriptor_header *) &ep_hs_int_desc,
    NULL,
};

static struct usb_composite_driver wifi_driver = {
    .name = "WiFi Adapter",
    .dev = &device_desc,
    .strings = dev_strings,
    .max_speed = USB_SPEED_SUPER,
    .bind = gwifi_composite_bind,
    .unbind = gwifi_composite_unbind,
};

#define func_to_gwifi_device(f) container_of(f, struct gwifi, func)

/**
 * @brief   - Allocate resouces per configuration's function. We have only one
 *            function (interface) for WiFi. This function initializes the
 *            function's endpoints: Tx, Rx, and Events.
 * @brief   - This is called when we add the function to the configuration by
 *            usb_add_function(). Initialize steps:
 *            1. Allocate the interface ID for that function.
 *            2. Allocate interface's endpoints.
 *            3. Assign the HS/SS descriptors to function.
 *            4. Initialize function specific resources.
 *
 * @todo    - Implement more interfaces for diagnostic, firmware update, etc.
 */
static int gwifi_bind(struct usb_configuration *config,
    struct usb_function *func)
{
    int ret;
    struct usb_ep *ep;
    struct usb_composite_dev *cdev = config->cdev;
    struct gwifi *dev = func_to_gwifi_device(func);

    /* 1. Allocate interface. */

    /* 2. Allocate endpoints. */
    ep = usb_ep_autoconfig(cdev->gadget, &ep_fs_out_desc);
    if (!ep)
    {
        pr_err("Failed to config Endpoint OUT.\n");
        return -ENODEV;
    }
    dev->ep_out = ep;

    ep = usb_ep_autoconfig(cdev->gadget, &ep_fs_in_desc);
    if (!ep)
    {
        pr_err("Failed to config Endpoint IN.\n");
        return -ENODEV;
    }
    dev->ep_in = ep;

    ep = usb_ep_autoconfig(cdev->gadget, &ep_fs_int_desc);
    if (!ep)
    {
        pr_err("Failed to config Endpoint INT.\n");
        return -ENODEV;
    }
    dev->ep_int = ep;

    /* Assign descriptors to function. */
    /* Note that we don't need to duplicate usb_ep_autoconfig() for HS because
     * only one speed descriptor will be selected. So what we do here is point 
     * them to the same allocated endpoint address, and the difference is only
     * wMaxPacketSize. */
    ep_hs_out_desc.bEndpointAddress = ep_fs_out_desc.bEndpointAddress;
    ep_hs_in_desc.bEndpointAddress = ep_fs_in_desc.bEndpointAddress;
    ep_hs_int_desc.bEndpointAddress = ep_fs_int_desc.bEndpointAddress;

    usb_assign_descriptors(func, fs_func_desc, hs_func_desc, NULL, NULL);

    return ret;
}

static void gwifi_unbind(struct usb_configuration *config,
      struct usb_function *func)
{
    
}

/**
 * @brief - Called when:
            1. USB cable connecred + host enumration complete.
            2. Changing Alternate Settings. Host send SET_INTERFACE.
          - We enable interface here.
 */
static int gwifi_set_alt(struct usb_function *func,
       unsigned interface, unsigned alt)
{
    int ret;

    return ret;
}

static void gwifi_disable(struct usb_function *func)
{

}

/**
 * @brief   - Is called when the module is registered to the kernel.
 *          - Used to allocate device resources.
 * @brief   - Init steps:
 *            1. Allocate the string descriptors, that will expose the device
 *               info to the host.
 *            2. Allocate a configuration.
 *            3. Allocate configuration's functions.
 *            4. Register the configuration with USB composite core.
 */
static int gwifi_composite_bind(struct usb_composite_dev *cdev)
{
    int ret = 0;
    struct usb_configuration *configuration;
    struct gwifi *dev;

    /* 1. Allocate the string descriptors. */
	ret = usb_string_ids_tab(cdev, strings_dev);
	if (ret < 0)
    {
        pr_err("Failed to get string id table: %d\n", ret);
        goto fail;
    }

    device_desc.iManufacturer = strings_dev[USB_GADGET_MANUFACTURER_IDX].id;
    device_desc.iProduct = strings_dev[USB_GADGET_PRODUCT_IDX].id;
    device_desc.iSerialNumber = strings_dev[USB_GADGET_SERIAL_IDX].id;

    /* 2. Allocation USB function. */
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
    {
        return -ENOMEM;
    }
    dev->func.name = "Gadge WiFi Function";
    dev->func.bind = gwifi_bind;
    dev->func.unbind = gwifi_unbind;
    dev->func.set_alt = gwifi_set_alt;
    dev->func.disable = gwifi_disable;

    /* 2. Allocate the USB configuration layer. */
    configuration = kzalloc(sizeof(*configuration), GFP_KERNEL);
    if (!configuration)
    {
        goto free_func;
        return -ENOMEM;
    }

    configuration->label = "EmbeddedOS USB Configuration";
    configuration->bConfigurationValue = 1;                  /* 1 Interface for this configuration. */
    configuration->bmAttributes = USB_CONFIG_ATT_ONE |       /* Mandatory. */
                                  USB_CONFIG_ATT_SELFPOWER | /* Self-powered device. */
                                  USB_CONFIG_ATT_WAKEUP;     /* Able to wake up host. */
    configuration->MaxPower = 1;                             /* 2mAh since we are self-power device. */

    /* 3.  Add function to configuration, called func bind(). */
    ret = usb_add_function(configuration, &dev->func);
    if (ret)
    {
        pr_err("Failed to add config to function: %d\n", ret);
        goto free_dev;
    }

    /* 4. Register configuration with composite core. */
    ret = usb_add_config(cdev, configuration, NULL);
    if (ret)
    {
        pr_err("Failed to add config to composite core: %d\n", ret);
        goto free_dev;
    }

    pr_info("Initialized USB configuration.\n");

    goto done;

free_dev:
    kfree(configuration);
free_func:
    kfree(dev);

done:
fail:
    return ret;
}

/**
 * @brief   - Is called when the module is unregistered from the kernel.
 *          - Used to undo what composite bind function did.
 */
static int gwifi_composite_unbind(struct usb_composite_dev *cdev)
{
    int ret = 0;
    pr_info("Unbind\n");

    return ret;
}

module_usb_composite_driver(wifi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeddedOS");
MODULE_DESCRIPTION("USB WiFi Adapter");