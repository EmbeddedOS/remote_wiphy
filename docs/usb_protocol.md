# USB Protocol

## USB Speed Modes and Descriptor Negotiation

USB devices can operate at different speeds depending on the capabilities of both the device and the host controller. The device must provide separate descriptors for each supported speed because the maximum packet sizes and timing parameters differ.

### USB Speed Classifications

| Speed | Standard | Data Rate | Bulk Max Packet Size | Typical Use |
|-------|----------|-----------|---------------------|-------------|
| **LS** (Low-Speed) | USB 1.0 (1996) | 1.5 Mbps | 8 bytes | Keyboards, mice |
| **FS** (Full-Speed) | USB 1.1 (1998) | 12 Mbps | 64 bytes | Audio devices, simple peripherals |
| **HS** (High-Speed) | USB 2.0 (2000) | 480 Mbps | 512 bytes | WiFi dongles, cameras, storage |
| **SS** (Super-Speed) | USB 3.0 (2008) | 5 Gbps | 1024 bytes | High-performance storage, WiFi 6 |
| **SSP** (Super-Speed+) | USB 3.1/3.2 | 10-20 Gbps | 1024 bytes | Latest high-bandwidth devices |

### Speed Negotiation Process

When a USB device is connected, the host and device negotiate the highest mutually supported speed:

```
1. Physical Connection
   └─ Host detects device presence on USB bus
   
2. Electrical Detection (Hardware Level)
   ├─ Device pulls D+ or D- line high (indicates speed capability)
   ├─ LS device: D- pulled high
   ├─ FS device: D+ pulled high
   └─ HS/SS device: Starts at FS, then chirps to request higher speed
   
3. Speed Negotiation
   ├─ USB 2.0: Device sends "chirp" signal to request HS mode
   ├─ USB 3.0+: Device negotiates via SuperSpeed signaling on extra pins
   └─ Host responds if it supports the requested speed
   
4. Descriptor Exchange (Selected Speed)
   ├─ Host: GET_DESCRIPTOR (Device)
   ├─ Device: Returns device descriptor
   ├─ Host: GET_DESCRIPTOR (Configuration)
   ├─ Device: Returns configuration with SPEED-SPECIFIC endpoint descriptors
   └─ Host loads appropriate driver
```

### Why Multiple Descriptor Sets?

The **same hardware endpoint** operates differently at each speed:

```c
/* Full-Speed Bulk OUT Endpoint */
struct usb_endpoint_descriptor fs_ep_out = {
    .bEndpointAddress = 0x01,        // Same address
    .bmAttributes = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize = cpu_to_le16(64),      // 64 bytes at FS
};

/* High-Speed Bulk OUT Endpoint */  
struct usb_endpoint_descriptor hs_ep_out = {
    .bEndpointAddress = 0x01,        // Same address!
    .bmAttributes = USB_ENDPOINT_XFER_BULK,
    .wMaxPacketSize = cpu_to_le16(512),     // 512 bytes at HS
};
```

**Key Point:** The endpoint address (0x01) is identical, only the packet size changes!

### Descriptor Assignment in Code

```c
/* In gwifi_bind() - called during module initialization */

/* Step 1: Allocate endpoints (assigns addresses) */
dev->ep_out = usb_ep_autoconfig(cdev->gadget, &fs_ep_out_desc);
// This updates fs_ep_out_desc.bEndpointAddress with hardware address

/* Step 2: Copy address to high-speed descriptor */
hs_ep_out_desc.bEndpointAddress = fs_ep_out_desc.bEndpointAddress;

/* Step 3: Tell kernel about both descriptor sets */
usb_assign_descriptors(func,
    fs_descriptors,  // Used if negotiated speed is FS
    hs_descriptors,  // Used if negotiated speed is HS
    ss_descriptors,  // Used if negotiated speed is SS
    NULL);

/* Step 4: Kernel automatically selects correct set during enumeration */
```

### What Happens During Enumeration

```
Host: "What speed can you do?"
Device: "I support HS (480 Mbps)"

Host: "Great! Show me your descriptors"
Device: [Kernel sends hs_descriptors automatically]

Host: "I see endpoint 0x01 with 512-byte packets. Perfect!"
Device: [Communication proceeds at HS with 512-byte packets]
```

If the host only supports FS:

```
Host: "I only support FS (12 Mbps)"
Device: [Kernel sends fs_descriptors automatically]

Host: "I see endpoint 0x01 with 64-byte packets. OK."
Device: [Communication proceeds at FS with 64-byte packets]
```

### Practical Example for WiFi

Our Remote WiPhy device supports both FS and HS:

**At Full-Speed (FS - 12 Mbps):**
- Bulk packet size: 64 bytes
- Effective throughput: ~1 MB/s
- Suitable for: Legacy systems, basic WiFi operations

**At High-Speed (HS - 480 Mbps):**
- Bulk packet size: 512 bytes  
- Effective throughput: ~40 MB/s
- Suitable for: Modern WiFi dongles (802.11n/ac)

The device automatically adapts to whatever speed the host negotiates!

### Common Misconception

**WRONG:** "I need separate endpoints for FS and HS"
```c
dev->ep_out_fs = usb_ep_autoconfig(..., &fs_ep_out_desc);  // ❌ NO!
dev->ep_out_hs = usb_ep_autoconfig(..., &hs_ep_out_desc);  // ❌ NO!
```

**CORRECT:** "I allocate endpoint once, then describe it at different speeds"
```c
/* Allocate ONCE */
dev->ep_out = usb_ep_autoconfig(cdev->gadget, &fs_ep_out_desc);

/* Copy address to HS descriptor */
hs_ep_out_desc.bEndpointAddress = fs_ep_out_desc.bEndpointAddress;

/* Now dev->ep_out works at both FS and HS! */
```

### Implementation Checklist

For your `g_wifi.c`, you need:

- [x] FS endpoint descriptors (already have)
- [ ] HS endpoint descriptors (need to add - same addresses, larger packets)
- [ ] Descriptor header arrays (fs_descriptors, hs_descriptors)
- [ ] Allocate endpoints ONCE in bind() using usb_ep_autoconfig()
- [ ] Copy endpoint addresses to HS descriptors in bind()
- [ ] Call usb_assign_descriptors() in bind() with both sets

---

## USB Requests - The Foundation of Data Transfer

### What is a USB Request?

A **USB request** (`struct usb_request`) is the fundamental unit of data transfer in the USB gadget framework. Think of it as a "data transfer operation" or "packet envelope" that moves data between the host and device.

**Key Concept:** Hardware endpoints are just **pipes** - they have no memory buffers. USB requests provide the buffers and instructions for transfers.

### The Analogy

```
Endpoint = Mailbox (just a location, no storage)
Request  = Envelope with letter inside
  ├─ buf       = The letter (actual data)
  ├─ length    = How many pages to send/receive
  ├─ complete  = What to do when mail is delivered
  └─ actual    = How many pages actually arrived
```

### USB Request Structure

```c
struct usb_request {
    /* Data buffer */
    void *buf;                   // Memory buffer for data
    unsigned int length;         // Requested transfer size
    dma_addr_t dma;              // DMA address (kernel manages)
    
    /* Transfer results (filled by kernel after completion) */
    unsigned int actual;         // Actual bytes transferred
    int status;                  // Transfer status (0 = success)
    
    /* Completion callback */
    void (*complete)(struct usb_ep *ep, struct usb_request *req);
    
    /* Context pointer */
    void *context;               // Your custom data (typically device struct)
    
    /* Internal fields managed by kernel */
    // ... more fields
};
```

### Why Do We Need USB Requests?

**Problem:** Endpoints don't have built-in buffers.

```
Hardware Endpoint (struct usb_ep *ep):
  ┌──────────────────────┐
  │   No buffer!         │  ← Just a hardware channel
  │   No memory!         │  ← Needs external buffer
  │   No instructions!   │  ← Needs completion callback
  └──────────────────────┘
```

**Solution:** Allocate USB requests to provide buffers and instructions.

```
USB Request (struct usb_request):
  ┌──────────────────────┐
  │ buf = kmalloc(512)   │  ← Actual data buffer
  │ length = 512         │  ← Transfer size
  │ complete = callback  │  ← What to do when done
  │ context = my_dev     │  ← My device data
  └──────────────────────┘
           ↓
    Attached to endpoint
           ↓
  ┌──────────────────────┐
  │    Hardware EP       │  ← Now knows where to put data!
  └──────────────────────┘
```

### USB Request Lifecycle

```
┌─────────────────────────────────────────────────────────┐
│                USB Request Lifecycle                     │
└─────────────────────────────────────────────────────────┘

1. ALLOCATE (in set_alt callback):
   ┌─────────────────────────────────────────┐
   │ req = usb_ep_alloc_request(ep, GFP_ATOMIC); │
   │ req->buf = kmalloc(512, GFP_ATOMIC);    │
   └─────────────────────────────────────────┘

2. CONFIGURE:
   ┌─────────────────────────────────────────┐
   │ req->length = 512;                      │
   │ req->complete = my_completion_callback; │
   │ req->context = dev;                     │
   └─────────────────────────────────────────┘

3. QUEUE (submit for transfer):
   ┌─────────────────────────────────────────┐
   │ usb_ep_queue(ep, req, GFP_ATOMIC);      │
   └─────────────────────────────────────────┘
        ↓
   [Hardware performs transfer...]
        ↓
4. COMPLETE (callback invoked automatically):
   ┌─────────────────────────────────────────┐
   │ my_completion_callback(ep, req) {       │
   │   // Check status                       │
   │   if (req->status == 0) {               │
   │     // Process req->buf data            │
   │     // req->actual = bytes transferred  │
   │   }                                     │
   │   // Re-queue for next transfer         │
   │   usb_ep_queue(ep, req, GFP_ATOMIC);    │
   │ }                                       │
   └─────────────────────────────────────────┘

5. FREE (in disable callback):
   ┌─────────────────────────────────────────┐
   │ usb_ep_dequeue(ep, req);                │
   │ kfree(req->buf);                        │
   │ usb_ep_free_request(ep, req);           │
   └─────────────────────────────────────────┘
```

### When to Allocate USB Requests

**In `set_alt()` callback** - NOT in `bind()`!

```c
/* WRONG - Don't allocate in bind() */
static int gwifi_bind(...)
{
    dev->req = usb_ep_alloc_request(dev->ep_out, GFP_KERNEL);  // ❌ TOO EARLY!
}

/* CORRECT - Allocate in set_alt() */
static int gwifi_set_alt(struct usb_function *func, unsigned intf, unsigned alt)
{
    struct gwifi *dev = func_to_gwifi_device(func);
    
    /* 1. Enable endpoint first */
    usb_ep_enable(dev->ep_out);
    
    /* 2. Then allocate request */
    dev->req_out = usb_ep_alloc_request(dev->ep_out, GFP_ATOMIC);  // ✅ CORRECT!
    if (!dev->req_out)
        return -ENOMEM;
    
    /* 3. Allocate buffer */
    dev->req_out->buf = kmalloc(512, GFP_ATOMIC);
    if (!dev->req_out->buf) {
        usb_ep_free_request(dev->ep_out, dev->req_out);
        return -ENOMEM;
    }
    
    /* 4. Configure request */
    dev->req_out->length = 512;
    dev->req_out->complete = gwifi_out_complete;
    dev->req_out->context = dev;
    
    /* 5. Queue to start receiving */
    return usb_ep_queue(dev->ep_out, dev->req_out, GFP_ATOMIC);
}
```

**Why in `set_alt()` and not `bind()`?**
- `bind()` happens at module load (USB might not be connected)
- `set_alt()` happens when USB cable is connected and host is ready
- Endpoints must be **enabled** before allocating requests

### Example: Receiving Data (OUT Endpoint)

```c
/* Allocate and start receiving */
static int gwifi_set_alt(struct usb_function *func, unsigned intf, unsigned alt)
{
    struct gwifi *dev = func_to_gwifi_device(func);
    
    /* Enable OUT endpoint */
    usb_ep_enable(dev->ep_out);
    
    /* Allocate request */
    dev->req_out = usb_ep_alloc_request(dev->ep_out, GFP_ATOMIC);
    dev->req_out->buf = kmalloc(512, GFP_ATOMIC);
    dev->req_out->length = 512;
    dev->req_out->complete = gwifi_out_complete;
    dev->req_out->context = dev;
    
    /* Start receiving */
    usb_ep_queue(dev->ep_out, dev->req_out, GFP_ATOMIC);
    //           ↑
    //           "Please receive up to 512 bytes into req->buf"
    
    return 0;
}

/* Called when host sends data */
static void gwifi_out_complete(struct usb_ep *ep, struct usb_request *req)
{
    struct gwifi *dev = req->context;
    
    switch (req->status) {
    case 0:  /* Normal completion */
        pr_info("Received %u bytes from host\n", req->actual);
        
        /* Data is now in req->buf, process it */
        print_hex_dump(KERN_INFO, "RX: ", DUMP_PREFIX_OFFSET,
                      16, 1, req->buf, req->actual, true);
        
        /* Your processing logic here */
        process_received_data(dev, req->buf, req->actual);
        
        /* Re-queue to receive more data */
        usb_ep_queue(ep, req, GFP_ATOMIC);
        break;
        
    case -ECONNRESET:
    case -ESHUTDOWN:
        /* USB disconnected, don't re-queue */
        pr_info("USB disconnected\n");
        break;
        
    default:
        pr_err("Transfer error: %d\n", req->status);
        /* Re-queue despite error */
        usb_ep_queue(ep, req, GFP_ATOMIC);
        break;
    }
}
```

### Example: Sending Data (IN Endpoint)

```c
/* Send data to host */
void gwifi_send_to_host(struct gwifi *dev, u8 *data, u16 len)
{
    /* Copy data to request buffer */
    memcpy(dev->req_in->buf, data, len);
    dev->req_in->length = len;
    
    /* Submit to IN endpoint */
    int ret = usb_ep_queue(dev->ep_in, dev->req_in, GFP_ATOMIC);
    //                      ↑
    //                      "Please send 'len' bytes to host"
    if (ret < 0) {
        pr_err("Failed to queue IN request: %d\n", ret);
    }
}

/* Called when host receives the data */
static void gwifi_in_complete(struct usb_ep *ep, struct usb_request *req)
{
    struct gwifi *dev = req->context;
    
    if (req->status == 0) {
        pr_info("Successfully sent %u bytes to host\n", req->actual);
    } else {
        pr_err("Send failed: %d\n", req->status);
    }
    
    /* For IN endpoint, typically don't re-queue automatically */
    /* Queue only when you have more data to send */
}
```

### Cleanup: Free Requests in disable()

```c
static void gwifi_disable(struct usb_function *func)
{
    struct gwifi *dev = func_to_gwifi_device(func);
    
    pr_info("Disabling endpoints and cleaning up\n");
    
    /* Stop OUT endpoint */
    if (dev->ep_out && dev->req_out) {
        usb_ep_dequeue(dev->ep_out, dev->req_out);  // Cancel pending transfer
        kfree(dev->req_out->buf);                   // Free buffer
        usb_ep_free_request(dev->ep_out, dev->req_out);  // Free request
        dev->req_out = NULL;
        usb_ep_disable(dev->ep_out);
    }
    
    /* Stop IN endpoint */
    if (dev->ep_in && dev->req_in) {
        usb_ep_dequeue(dev->ep_in, dev->req_in);
        kfree(dev->req_in->buf);
        usb_ep_free_request(dev->ep_in, dev->req_in);
        dev->req_in = NULL;
        usb_ep_disable(dev->ep_in);
    }
    
    /* Stop INT endpoint */
    if (dev->ep_int && dev->req_int) {
        usb_ep_dequeue(dev->ep_int, dev->req_int);
        kfree(dev->req_int->buf);
        usb_ep_free_request(dev->ep_int, dev->req_int);
        dev->req_int = NULL;
        usb_ep_disable(dev->ep_int);
    }
}
```

### Common Request Patterns

#### Pattern 1: Continuous Reception (OUT endpoint)
```c
/* OUT completion callback - keep receiving */
static void out_complete(struct usb_ep *ep, struct usb_request *req)
{
    if (req->status == 0) {
        process_data(req->buf, req->actual);
    }
    
    /* Always re-queue to keep receiving */
    usb_ep_queue(ep, req, GFP_ATOMIC);
}
```

#### Pattern 2: On-Demand Transmission (IN endpoint)
```c
/* Only queue when you have data to send */
void send_when_ready(struct gwifi *dev, u8 *data, u16 len)
{
    if (dev->req_in_busy)
        return;  /* Previous transfer still pending */
    
    memcpy(dev->req_in->buf, data, len);
    dev->req_in->length = len;
    dev->req_in_busy = true;
    
    usb_ep_queue(dev->ep_in, dev->req_in, GFP_ATOMIC);
}

static void in_complete(struct usb_ep *ep, struct usb_request *req)
{
    struct gwifi *dev = req->context;
    dev->req_in_busy = false;  /* Now available for next send */
}
```

#### Pattern 3: Event Notification (Interrupt endpoint)
```c
/* Send async event to host */
void notify_host_event(struct gwifi *dev, u8 event_type)
{
    struct event_packet *evt = (struct event_packet *)dev->req_int->buf;
    
    evt->type = event_type;
    evt->timestamp = jiffies;
    
    dev->req_int->length = sizeof(*evt);
    usb_ep_queue(dev->ep_int, dev->req_int, GFP_ATOMIC);
}
```

### Request Status Codes

Common values for `req->status` in completion callback:

| Status | Meaning |
|--------|---------|
| `0` | Success - transfer completed normally |
| `-ECONNRESET` | Connection reset - USB cable unplugged |
| `-ESHUTDOWN` | Endpoint disabled - device shutting down |
| `-EOVERFLOW` | Received more data than buffer size |
| `-EREMOTEIO` | Short packet (less data than expected) |
| `-EPROTO` | Protocol error |
| `-EILSEQ` | Illegal sequence error |

### Multiple Requests Per Endpoint (Advanced)

For high-throughput applications, you can allocate **multiple requests** per endpoint to pipeline transfers:

```c
#define NUM_REQUESTS 4

struct gwifi {
    struct usb_function func;
    struct usb_ep *ep_out;
    struct usb_request *req_out[NUM_REQUESTS];  // Multiple requests!
};

static int gwifi_set_alt(...)
{
    /* Allocate multiple requests */
    for (int i = 0; i < NUM_REQUESTS; i++) {
        dev->req_out[i] = usb_ep_alloc_request(dev->ep_out, GFP_ATOMIC);
        dev->req_out[i]->buf = kmalloc(512, GFP_ATOMIC);
        dev->req_out[i]->length = 512;
        dev->req_out[i]->complete = out_complete;
        
        /* Queue all of them */
        usb_ep_queue(dev->ep_out, dev->req_out[i], GFP_ATOMIC);
    }
    
    /* Now 4 transfers can be in-flight simultaneously! */
}
```

### Key Takeaways

1. **USB Request** = Transfer operation with buffer + completion callback
2. **Allocate in `set_alt()`** - After endpoints are enabled
3. **Free in `disable()`** - Clean up when USB disconnects
4. **OUT pattern** - Queue once, re-queue in completion (continuous receive)
5. **IN pattern** - Queue on-demand when you have data to send
6. **Check `req->status`** - Always handle errors in completion callback
7. **Use `req->actual`** - Actual bytes transferred (may be less than `length`)

**Remember:** Endpoint = pipe, Request = envelope. You need both to transfer data!

---

## Power (V Bus)

USB is bus-powered devices and a USB devce specifies its power consumption expressed in 2mA units in the configuration descriptor. A device CANNOT increase its power consumption, greater than what it specifies during enumration.