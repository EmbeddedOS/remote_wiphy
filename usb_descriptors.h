#pragma once

/* Device Descriptor:
 *   idVendor: 0xffff
 *   idProduct: 0xffff
 *   bcdDevice: 0x0100
 *   iManufacturer: "EmbeddedOS"
 *   iProduct: "EmbeddedOS Remote WiPhy over USB"
 *   iSerial: "rw-0001"
 * 
 * Configuration (1):
 *   bNumInterfaces: 1
 *   bmAttributes: 0x80
 *   MaxPower: 2mA
 * 
 * Interface 0:
 *   bInterfaceClass: 0xFF (Vendor specific)
 *   bInterfaceSubClass: 0x00
 *   bInterfaceProtocol: 0x00
 *   bNumEndpoints: 3
 * 
 * Endpoint 1:  EP1 OUT  (Bulk)  Addr: 0x01  wMaxPacketSize: 512 (HS)
 * Endpoint 2:  EP1 IN   (Bulk)  Addr: 0x81  wMaxPacketSize: 512 (HS)
 * Endpoint 3:  EP2 IN   (Interrupt) Addr: 0x82 wMaxPacketSize:64 (HS) interval 1ms
 */

#define REMOTE_WIPHY_PRODUCT_ID 0xFFFF
#define REMOTE_WIPHY_VENDOR_ID 0xFFFF
#define REMOTE_WIPHY_DEVICE_VERSION 0x0100 /* Version 1.0 */
#define REMOTE_WIPHY_MANUFACTURER "EmbeddedOS"
#define REMOTE_WIPHY_PRODUCT "EmbeddedOS Remote WiPhy over USB"
#define REMOTE_WIPHY_SERIAL "rw-0001"
#define REMOTE_WIPHY_CONFIGURATION_MAX_POWER 250