/* USB Controller Header for Raspberry Pi */

#ifndef USB_H
#define USB_H

#include <stdint.h>

// USB xHCI Controller registers (Pi 4)
#define USB_BASE 0xFE9C0000

#define USB_CAPLENGTH    (USB_BASE + 0x00)  // Capability Registers Length
#define USB_HCIVERSION   (USB_BASE + 0x02)  // Interface Version Number
#define USB_HCSPARAMS1   (USB_BASE + 0x04)  // Structural Parameters 1
#define USB_HCSPARAMS2   (USB_BASE + 0x08)  // Structural Parameters 2
#define USB_HCSPARAMS3   (USB_BASE + 0x0C)  // Structural Parameters 3
#define USB_HCCPARAMS1   (USB_BASE + 0x10)  // Capability Parameters 1
#define USB_DBOFF        (USB_BASE + 0x14)  // Doorbell Offset
#define USB_RTSOFF       (USB_BASE + 0x18)  // Runtime Register Space Offset
#define USB_HCCPARAMS2   (USB_BASE + 0x1C)  // Capability Parameters 2

// Operational Registers
#define USB_USBCMD       (USB_BASE + 0x20)  // USB Command
#define USB_USBSTS       (USB_BASE + 0x24)  // USB Status
#define USB_PAGESIZE     (USB_BASE + 0x28)  // Page Size
#define USB_DNCTRL       (USB_BASE + 0x34)  // Device Notification Control
#define USB_CRCR         (USB_BASE + 0x38)  // Command Ring Control
#define USB_DCBAAP       (USB_BASE + 0x50)  // Device Context Base Address Array Pointer
#define USB_CONFIG       (USB_BASE + 0x58)  // Configure

// USB Command register bits
#define USB_CMD_RS       (1 << 0)   // Run/Stop
#define USB_CMD_HCRST    (1 << 1)   // Host Controller Reset
#define USB_CMD_INTE     (1 << 2)   // Interrupter Enable
#define USB_CMD_HSEE     (1 << 3)   // Host System Error Enable

// USB Status register bits
#define USB_STS_HCH      (1 << 0)   // Host Controller Halted
#define USB_STS_HSE      (1 << 2)   // Host System Error
#define USB_STS_EINT     (1 << 3)   // Event Interrupt
#define USB_STS_PCD      (1 << 4)   // Port Change Detect
#define USB_STS_SSS      (1 << 8)   // Save State Status
#define USB_STS_RSS      (1 << 9)   // Restore State Status
#define USB_STS_SRE      (1 << 10)  // Save/Restore Error
#define USB_STS_CNR      (1 << 11)  // Controller Not Ready
#define USB_STS_HCE      (1 << 12)  // Host Controller Error

// Port Status and Control
#define USB_PORTSC1      (USB_BASE + 0x420)  // Port 1 Status and Control
#define USB_PORTSC2      (USB_BASE + 0x430)  // Port 2 Status and Control

// Port Status bits
#define PORT_CCS         (1 << 0)   // Current Connect Status
#define PORT_PE          (1 << 1)   // Port Enabled
#define PORT_OCA         (1 << 3)   // Over-current Active
#define PORT_RESET       (1 << 4)   // Port Reset
#define PORT_LS_MASK     (0xF << 5) // Link State
#define PORT_PP          (1 << 9)   // Port Power
#define PORT_CSC         (1 << 17)  // Connect Status Change
#define PORT_PEC         (1 << 18)  // Port Enabled/Disabled Change
#define PORT_OCC         (1 << 20)  // Over-current Change

// USB transfer types
#define USB_CONTROL      0
#define USB_ISOCHRONOUS  1
#define USB_BULK         2
#define USB_INTERRUPT    3

// USB device states
#define USB_DEVICE_ATTACHED    0
#define USB_DEVICE_POWERED     1
#define USB_DEVICE_DEFAULT     2
#define USB_DEVICE_ADDRESS     3
#define USB_DEVICE_CONFIGURED  4

// USB request types
#define USB_REQ_TYPE_STANDARD  0x00
#define USB_REQ_TYPE_CLASS     0x20
#define USB_REQ_TYPE_VENDOR    0x40
#define USB_REQ_TYPE_MASK      0x60

#define USB_REQ_RECIPIENT_DEVICE    0x00
#define USB_REQ_RECIPIENT_INTERFACE 0x01
#define USB_REQ_RECIPIENT_ENDPOINT  0x02
#define USB_REQ_RECIPIENT_OTHER     0x03
#define USB_REQ_RECIPIENT_MASK      0x1F

// Standard USB requests
#define USB_REQ_GET_STATUS        0x00
#define USB_REQ_CLEAR_FEATURE     0x01
#define USB_REQ_SET_FEATURE       0x03
#define USB_REQ_SET_ADDRESS       0x05
#define USB_REQ_GET_DESCRIPTOR    0x06
#define USB_REQ_SET_DESCRIPTOR    0x07
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09
#define USB_REQ_GET_INTERFACE     0x0A
#define USB_REQ_SET_INTERFACE     0x0B
#define USB_REQ_SYNCH_FRAME       0x0C

// Descriptor types
#define USB_DESC_DEVICE           0x01
#define USB_DESC_CONFIGURATION    0x02
#define USB_DESC_STRING           0x03
#define USB_DESC_INTERFACE        0x04
#define USB_DESC_ENDPOINT         0x05
#define USB_DESC_DEVICE_QUALIFIER 0x06
#define USB_DESC_OTHER_SPEED      0x07
#define USB_DESC_INTERFACE_POWER  0x08

// USB device descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

// USB configuration descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

// USB interface descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed)) usb_interface_descriptor_t;

// USB endpoint descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

// Mass Storage Class specific
#define USB_CLASS_MASS_STORAGE  0x08
#define USB_SUBCLASS_SCSI       0x06
#define USB_PROTOCOL_BULK_ONLY  0x50

// SCSI commands
#define SCSI_TEST_UNIT_READY    0x00
#define SCSI_REQUEST_SENSE      0x03
#define SCSI_INQUIRY            0x12
#define SCSI_READ_CAPACITY      0x25
#define SCSI_READ_10            0x28
#define SCSI_WRITE_10           0x2A

typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_packet_t;

// USB device structure
typedef struct {
    uint8_t address;
    uint8_t state;
    usb_device_descriptor_t descriptor;
    uint8_t config_index;
    uint8_t interface_index;
    uint8_t bulk_in_ep;
    uint8_t bulk_out_ep;
    uint16_t max_packet_size;
} usb_device_t;

// USB transfer structure
typedef struct {
    uint8_t *buffer;
    uint32_t length;
    uint32_t transferred;
    int status;
} usb_transfer_t;

// Basic controller functions
void usb_init(void);
int usb_reset_controller(void);
int usb_start_controller(void);
int usb_stop_controller(void);
uint32_t usb_get_port_status(uint8_t port);
int usb_reset_port(uint8_t port);
int usb_enable_port(uint8_t port);

// USB protocol functions
int usb_enumerate_device(uint8_t port);
int usb_get_descriptor(uint8_t address, uint8_t type, uint8_t index, uint8_t *buffer, uint16_t length);
int usb_set_address(uint8_t address);
int usb_set_configuration(uint8_t address, uint8_t config);
int usb_control_transfer(uint8_t address, usb_setup_packet_t *setup, uint8_t *data, uint16_t length);
int usb_bulk_transfer(uint8_t address, uint8_t endpoint, uint8_t *data, uint16_t length, int direction);

// Mass Storage Class functions
int usb_msc_init(uint8_t address);
int usb_msc_read_sector(uint8_t address, uint32_t sector, uint8_t *buffer);
int usb_msc_write_sector(uint8_t address, uint32_t sector, uint8_t *buffer);
int usb_msc_get_capacity(uint8_t address, uint32_t *sectors, uint32_t *sector_size);

// USB boot functions
int usb_boot_init(void);
int usb_boot_load_file(const char *filename, void *buffer, uint32_t max_size);

#endif