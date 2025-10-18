/* USB Controller Implementation for Raspberry Pi */

#include "usb.h"
#include "timer.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

// Helper functions
static void mmio_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t*)reg = data;
}

static uint32_t mmio_read(uint32_t reg) {
    return *(volatile uint32_t*)reg;
}

// USB device management
static usb_device_t usb_devices[4]; // Support up to 4 devices
static uint8_t next_address = 1;

// Transfer buffer (4KB aligned)
static uint8_t usb_transfer_buffer[4096] __attribute__((aligned(4096)));

// Custom memcpy for freestanding environment
static void *memcpy(void *dest, const void *src, uint32_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static void *memset(void *s, int c, uint32_t n) {
    unsigned char *p = s;
    while (n--) *p++ = c;
    return s;
}

void usb_init(void) {
    // Initialize USB device array
    memset(usb_devices, 0, sizeof(usb_devices));
    next_address = 1;

    // Basic xHCI controller initialization
    usb_reset_controller();

    // Wait for controller to be ready
    uint64_t timeout = timer_get_counter() + 1000000; // 1 second timeout
    while ((mmio_read(USB_USBSTS) & USB_STS_CNR) && (timer_get_counter() < timeout));

    if (mmio_read(USB_USBSTS) & USB_STS_CNR) {
        return; // Controller not ready
    }

    // Enable ports
    usb_enable_port(1);
    usb_enable_port(2);

    usb_start_controller();
}

int usb_reset_controller(void) {
    // Issue host controller reset
    mmio_write(USB_USBCMD, USB_CMD_HCRST);

    // Wait for reset to complete
    while (mmio_read(USB_USBSTS) & USB_STS_CNR);

    return 0;
}

int usb_start_controller(void) {
    // Set run/stop bit
    uint32_t cmd = mmio_read(USB_USBCMD);
    cmd |= USB_CMD_RS;
    mmio_write(USB_USBCMD, cmd);

    // Wait for controller to start
    while (mmio_read(USB_USBSTS) & USB_STS_HCH);

    return 0;
}

int usb_stop_controller(void) {
    // Clear run/stop bit
    uint32_t cmd = mmio_read(USB_USBCMD);
    cmd &= ~USB_CMD_RS;
    mmio_write(USB_USBCMD, cmd);

    // Wait for controller to halt
    while (!(mmio_read(USB_USBSTS) & USB_STS_HCH));

    return 0;
}

uint32_t usb_get_port_status(uint8_t port) {
    uint32_t port_reg;
    if (port == 1) {
        port_reg = USB_PORTSC1;
    } else if (port == 2) {
        port_reg = USB_PORTSC2;
    } else {
        return 0;
    }

    return mmio_read(port_reg);
}

int usb_reset_port(uint8_t port) {
    uint32_t port_reg;
    if (port == 1) {
        port_reg = USB_PORTSC1;
    } else if (port == 2) {
        port_reg = USB_PORTSC2;
    } else {
        return -1;
    }

    uint32_t status = mmio_read(port_reg);
    status |= PORT_RESET;
    mmio_write(port_reg, status);

    // Wait for reset to complete
    while (mmio_read(port_reg) & PORT_RESET);

    return 0;
}

int usb_enable_port(uint8_t port) {
    uint32_t port_reg;
    if (port == 1) {
        port_reg = USB_PORTSC1;
    } else if (port == 2) {
        port_reg = USB_PORTSC2;
    } else {
        return -1;
    }

    uint32_t status = mmio_read(port_reg);
    status |= PORT_PP;  // Port Power
    mmio_write(port_reg, status);

    return 0;
}

// USB protocol implementation
int usb_enumerate_device(uint8_t port) {
    uint32_t port_status = usb_get_port_status(port);
    if (!(port_status & PORT_CCS)) {
        return -1; // No device connected
    }

    // Reset port
    usb_reset_port(port);

    // Wait for device to be ready
    timer_delay_ms(100);

    // Get device descriptor
    usb_device_descriptor_t dev_desc;
    if (usb_get_descriptor(0, USB_DESC_DEVICE, 0, (uint8_t*)&dev_desc, sizeof(dev_desc)) < 0) {
        return -1;
    }

    // Set device address
    uint8_t address = next_address++;
    if (usb_set_address(address) < 0) {
        return -1;
    }

    // Store device info
    usb_devices[address].address = address;
    usb_devices[address].state = USB_DEVICE_ADDRESS;
    memcpy(&usb_devices[address].descriptor, &dev_desc, sizeof(dev_desc));
    usb_devices[address].max_packet_size = dev_desc.bMaxPacketSize0;

    // Get full device descriptor
    if (usb_get_descriptor(address, USB_DESC_DEVICE, 0, (uint8_t*)&usb_devices[address].descriptor, sizeof(dev_desc)) < 0) {
        return -1;
    }

    // Get configuration descriptor
    usb_config_descriptor_t config_desc;
    if (usb_get_descriptor(address, USB_DESC_CONFIGURATION, 0, (uint8_t*)&config_desc, sizeof(config_desc)) < 0) {
        return -1;
    }

    // Set configuration
    if (usb_set_configuration(address, config_desc.bConfigurationValue) < 0) {
        return -1;
    }

    usb_devices[address].state = USB_DEVICE_CONFIGURED;

    return address;
}

int usb_get_descriptor(uint8_t address, uint8_t type, uint8_t index, uint8_t *buffer, uint16_t length) {
    usb_setup_packet_t setup = {
        .bmRequestType = USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
        .bRequest = USB_REQ_GET_DESCRIPTOR,
        .wValue = (type << 8) | index,
        .wIndex = 0,
        .wLength = length
    };

    return usb_control_transfer(address, &setup, buffer, length);
}

int usb_set_address(uint8_t address) {
    usb_setup_packet_t setup = {
        .bmRequestType = USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
        .bRequest = USB_REQ_SET_ADDRESS,
        .wValue = address,
        .wIndex = 0,
        .wLength = 0
    };

    return usb_control_transfer(0, &setup, NULL, 0);
}

int usb_set_configuration(uint8_t address, uint8_t config) {
    usb_setup_packet_t setup = {
        .bmRequestType = USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
        .bRequest = USB_REQ_SET_CONFIGURATION,
        .wValue = config,
        .wIndex = 0,
        .wLength = 0
    };

    return usb_control_transfer(address, &setup, NULL, 0);
}

int usb_control_transfer(uint8_t address, usb_setup_packet_t *setup, uint8_t *data, uint16_t length) {
    // Simplified control transfer implementation
    // In a real implementation, this would use the xHCI transfer rings
    // For bootloader purposes, we'll simulate success

    if (address == 0) {
        // Default address - simulate device descriptor
        if (setup->bRequest == USB_REQ_GET_DESCRIPTOR && setup->wValue == 0x0100) {
            usb_device_descriptor_t *desc = (usb_device_descriptor_t*)data;
            desc->bLength = sizeof(usb_device_descriptor_t);
            desc->bDescriptorType = USB_DESC_DEVICE;
            desc->bcdUSB = 0x0200;
            desc->bDeviceClass = 0;
            desc->bDeviceSubClass = 0;
            desc->bDeviceProtocol = 0;
            desc->bMaxPacketSize0 = 64;
            desc->idVendor = 0x1234;
            desc->idProduct = 0x5678;
            desc->bcdDevice = 0x0100;
            desc->iManufacturer = 0;
            desc->iProduct = 0;
            desc->iSerialNumber = 0;
            desc->bNumConfigurations = 1;
            return sizeof(usb_device_descriptor_t);
        }
    }

    return length; // Simulate success
}

int usb_bulk_transfer(uint8_t address, uint8_t endpoint, uint8_t *data, uint16_t length, int direction) {
    // Simplified bulk transfer implementation
    // In a real implementation, this would use xHCI bulk transfer descriptors
    return length; // Simulate success
}

// Mass Storage Class implementation
int usb_msc_init(uint8_t address) {
    usb_device_t *device = &usb_devices[address];
    if (device->state != USB_DEVICE_CONFIGURED) {
        return -1;
    }

    // Get configuration descriptor to find MSC interface
    usb_config_descriptor_t config;
    if (usb_get_descriptor(address, USB_DESC_CONFIGURATION, 0, (uint8_t*)&config, sizeof(config)) < 0) {
        return -1;
    }

    // Parse configuration to find MSC interface and endpoints
    uint8_t buffer[256];
    if (usb_get_descriptor(address, USB_DESC_CONFIGURATION, 0, buffer, config.wTotalLength) < 0) {
        return -1;
    }

    uint8_t *ptr = buffer + sizeof(usb_config_descriptor_t);
    uint8_t *end = buffer + config.wTotalLength;

    while (ptr < end) {
        uint8_t desc_type = ptr[1];
        uint8_t desc_len = ptr[0];

        if (desc_type == USB_DESC_INTERFACE) {
            usb_interface_descriptor_t *iface = (usb_interface_descriptor_t*)ptr;
            if (iface->bInterfaceClass == USB_CLASS_MASS_STORAGE &&
                iface->bInterfaceSubClass == USB_SUBCLASS_SCSI &&
                iface->bInterfaceProtocol == USB_PROTOCOL_BULK_ONLY) {

                device->interface_index = iface->bInterfaceNumber;
                ptr += desc_len;

                // Find bulk endpoints
                for (int i = 0; i < iface->bNumEndpoints && ptr < end; i++) {
                    if (ptr[1] == USB_DESC_ENDPOINT) {
                        usb_endpoint_descriptor_t *ep = (usb_endpoint_descriptor_t*)ptr;
                        if ((ep->bmAttributes & 0x03) == USB_BULK) {
                            if (ep->bEndpointAddress & 0x80) {
                                device->bulk_in_ep = ep->bEndpointAddress;
                            } else {
                                device->bulk_out_ep = ep->bEndpointAddress;
                            }
                        }
                        ptr += ep->bLength;
                    } else {
                        ptr += ptr[0];
                    }
                }
                break;
            }
        }
        ptr += desc_len;
    }

    return (device->bulk_in_ep && device->bulk_out_ep) ? 0 : -1;
}

int usb_msc_read_sector(uint8_t address, uint32_t sector, uint8_t *buffer) {
    // SCSI READ(10) command
    uint8_t cdb[10] = {
        SCSI_READ_10,
        0, // Flags
        (sector >> 24) & 0xFF,
        (sector >> 16) & 0xFF,
        (sector >> 8) & 0xFF,
        sector & 0xFF,
        0, // Reserved
        0, // Transfer length (1 sector)
        1,
        0  // Control
    };

    // Send CDB via bulk out
    usb_bulk_transfer(address, usb_devices[address].bulk_out_ep, cdb, sizeof(cdb), 0);

    // Receive data via bulk in
    return usb_bulk_transfer(address, usb_devices[address].bulk_in_ep, buffer, 512, 1);
}

int usb_msc_write_sector(uint8_t address, uint32_t sector, uint8_t *buffer) {
    // SCSI WRITE(10) command
    uint8_t cdb[10] = {
        SCSI_WRITE_10,
        0, // Flags
        (sector >> 24) & 0xFF,
        (sector >> 16) & 0xFF,
        (sector >> 8) & 0xFF,
        sector & 0xFF,
        0, // Reserved
        0, // Transfer length (1 sector)
        1,
        0  // Control
    };

    // Send CDB via bulk out
    usb_bulk_transfer(address, usb_devices[address].bulk_out_ep, cdb, sizeof(cdb), 0);

    // Send data via bulk out
    return usb_bulk_transfer(address, usb_devices[address].bulk_out_ep, buffer, 512, 0);
}

int usb_msc_get_capacity(uint8_t address, uint32_t *sectors, uint32_t *sector_size) {
    uint8_t cdb[10] = {SCSI_READ_CAPACITY, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t response[8];

    // Send CDB
    usb_bulk_transfer(address, usb_devices[address].bulk_out_ep, cdb, sizeof(cdb), 0);

    // Get response
    if (usb_bulk_transfer(address, usb_devices[address].bulk_in_ep, response, sizeof(response), 1) != sizeof(response)) {
        return -1;
    }

    *sectors = (response[0] << 24) | (response[1] << 16) | (response[2] << 8) | response[3];
    *sector_size = (response[4] << 24) | (response[5] << 16) | (response[6] << 8) | response[7];

    return 0;
}

// USB boot functions
int usb_boot_init(void) {
    usb_init();

    // Enumerate devices on both ports
    for (uint8_t port = 1; port <= 2; port++) {
        int address = usb_enumerate_device(port);
        if (address > 0) {
            usb_device_t *device = &usb_devices[address];
            if (device->descriptor.bDeviceClass == USB_CLASS_MASS_STORAGE ||
                (device->descriptor.bDeviceClass == 0 && usb_msc_init(address) == 0)) {
                // Found mass storage device
                return address;
            }
        }
    }

    return -1; // No mass storage device found
}

int usb_boot_load_file(const char *filename, void *buffer, uint32_t max_size) {
    // For bootloader, we'll simulate loading from USB
    // In a real implementation, this would parse FAT filesystem on USB drive
    return -1; // Not implemented for simulation
}