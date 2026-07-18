#ifndef USB_REPLUG_USB_DESCRIPTORS_H
#define USB_REPLUG_USB_DESCRIPTORS_H

#include <stdint.h>

#ifdef __SDCC_mcs51
#define USB_CODE __code
#else
#define USB_CODE
#endif

#ifndef USB_REPLUG_VID
#define USB_REPLUG_VID 0x1A86u
#endif

#ifndef USB_REPLUG_PID
#define USB_REPLUG_PID 0x5722u
#endif

#define USB_DESCRIPTOR_DEVICE 0x01u
#define USB_DESCRIPTOR_CONFIGURATION 0x02u
#define USB_DESCRIPTOR_STRING 0x03u
#define USB_CDC_CONFIGURATION_LENGTH 75u
#define USB_SERIAL_DESCRIPTOR_LENGTH 32u

extern const uint8_t USB_CODE usb_device_descriptor[18];
extern const uint8_t USB_CODE usb_configuration_descriptor[USB_CDC_CONFIGURATION_LENGTH];

const uint8_t *usb_descriptor_lookup(
    uint8_t descriptor_type,
    uint8_t descriptor_index,
    uint16_t *length
);
uint8_t usb_build_serial_descriptor(uint8_t *destination, const uint8_t chip_id[5]);

#endif
