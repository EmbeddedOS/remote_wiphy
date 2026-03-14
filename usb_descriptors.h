#pragma once

/**
 * Remote WiPhy USB Device Descriptor
 * ===================================
 * 
 * This USB gadget exposes a WiFi Physical Layer (PHY) over USB, allowing a host
 * system to use a remote WiFi hardware device. The device integrates with the
 * Linux cfg80211/mac80211 wireless subsystem on the host side, providing full
 * WiFi functionality including scanning, association, monitor mode, and AP mode.
 *
 * Architecture:
 * -------------
 *   Device: Remote WiFi Adapter (Vendor-specific)
 *     └─── Configuration #1: WiFi Function
 *            └─── Interface #0: WiFi Control & Data
 *                   ├─── EP 0x01 (Bulk OUT): Commands + TX frames (multiplexed)
 *                   ├─── EP 0x81 (Bulk IN):  Responses + RX frames (multiplexed)
 *                   └─── EP 0x82 (Interrupt IN): Async events
 *
 * Endpoint Usage:
 * ---------------
 *   EP 0x01 OUT (Bulk, 512 bytes):
 *     - WiFi management commands (scan, associate, configure)
 *     - 802.11 TX frames from host to be transmitted over the air
 *     - Packet header distinguishes command vs data
 *
 *   EP 0x81 IN (Bulk, 512 bytes):
 *     - Command responses and status
 *     - 802.11 RX frames received over the air
 *     - Packet header distinguishes response vs data
 *
 *   EP 0x82 IN (Interrupt, 64 bytes, 1ms interval):
 *     - Asynchronous events (scan complete, link up/down, etc.)
 *     - Status notifications
 *
 * USB Descriptor Hierarchy:
 * -------------------------
 *   Device Descriptor:
 *     bcdUSB              : 0x0200 (USB 2.0)
 *     bDeviceClass        : 0xFF (Vendor-specific)
 *     idVendor            : 0xFFFF
 *     idProduct           : 0xFFFF
 *     bcdDevice           : 0x0100 (Version 1.0)
 *     iManufacturer       : "EmbeddedOS"
 *     iProduct            : "EmbeddedOS Remote WiPhy over USB"
 *     iSerialNumber       : "rw-0001"
 *     bNumConfigurations  : 1
 *
 *   Configuration Descriptor:
 *     bConfigurationValue : 1
 *     bNumInterfaces      : 1
 *     bmAttributes        : 0x80 (Bus-powered)
 *     bMaxPower           : 2mA
 *
 *   Interface Descriptor (Interface 0, Alternate Setting 0):
 *     bInterfaceNumber    : 0
 *     bInterfaceClass     : 0xFF (Vendor-specific)
 *     bInterfaceSubClass  : 0x00
 *     bInterfaceProtocol  : 0x00
 *     bNumEndpoints       : 3
 *
 *   Endpoint Descriptors:
 *     EP 0x01 OUT: Bulk, 512 bytes (HS), Host → Device
 *     EP 0x81 IN:  Bulk, 512 bytes (HS), Device → Host
 *     EP 0x82 IN:  Interrupt, 64 bytes (HS), Device → Host, 1ms interval
 *
 * Design Notes:
 * -------------
 * - Uses multiplexed bulk endpoints (commands + data on same pipe)
 * - Single interface design (can be extended to multiple interfaces later)
 * - Vendor-specific class allows custom protocol tailored for WiFi operations
 * - Future expansion possible: add debug interface, firmware update interface
 */

#define REMOTE_WIPHY_PRODUCT_ID 0xFFFF
#define REMOTE_WIPHY_VENDOR_ID 0xFFFF
#define REMOTE_WIPHY_DEVICE_VERSION 0x0100 /* Version 1.0 */
#define REMOTE_WIPHY_MANUFACTURER "EmbeddedOS"
#define REMOTE_WIPHY_PRODUCT "EmbeddedOS Remote WiPhy over USB"
#define REMOTE_WIPHY_SERIAL "rw-0001"
#define REMOTE_WIPHY_CONFIGURATION_MAX_POWER 250