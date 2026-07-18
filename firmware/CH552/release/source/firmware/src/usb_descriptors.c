#include "usb_descriptors.h"

#define USB_U16_LOW(value) ((uint8_t)((value) & 0xffu))
#define USB_U16_HIGH(value) ((uint8_t)(((value) >> 8) & 0xffu))

const uint8_t USB_CODE usb_device_descriptor[18] = {
    18u, USB_DESCRIPTOR_DEVICE,
    0x00u, 0x02u,
    0x02u, 0x00u, 0x00u,
    8u,
    USB_U16_LOW(USB_REPLUG_VID), USB_U16_HIGH(USB_REPLUG_VID),
    USB_U16_LOW(USB_REPLUG_PID), USB_U16_HIGH(USB_REPLUG_PID),
    0x00u, 0x02u,
    1u, 2u, 3u,
    1u
};

const uint8_t USB_CODE usb_configuration_descriptor[USB_CDC_CONFIGURATION_LENGTH] = {
    /* Configuration: two interfaces, bus powered, 100 mA. */
    9u, USB_DESCRIPTOR_CONFIGURATION,
    USB_U16_LOW(USB_CDC_CONFIGURATION_LENGTH),
    USB_U16_HIGH(USB_CDC_CONFIGURATION_LENGTH),
    2u, 1u, 0u, 0x80u, 50u,

    /* Interface association for the CDC ACM function. */
    8u, 0x0Bu, 0u, 2u, 0x02u, 0x02u, 0x01u, 4u,

    /* CDC communication/control interface. */
    9u, 0x04u, 0u, 0u, 1u, 0x02u, 0x02u, 0x01u, 4u,
    5u, 0x24u, 0x00u, 0x10u, 0x01u,
    5u, 0x24u, 0x01u, 0x00u, 1u,
    4u, 0x24u, 0x02u, 0x02u,
    5u, 0x24u, 0x06u, 0u, 1u,

    /* Optional CDC notification endpoint; firmware leaves it NAK-idle. */
    7u, 0x05u, 0x81u, 0x03u, 8u, 0u, 16u,

    /* CDC data interface with one bulk OUT and one bulk IN endpoint. */
    9u, 0x04u, 1u, 0u, 2u, 0x0Au, 0x00u, 0x00u, 4u,
    7u, 0x05u, 0x02u, 0x02u, 64u, 0u, 0u,
    7u, 0x05u, 0x82u, 0x02u, 64u, 0u, 0u
};

static const uint8_t USB_CODE usb_language_descriptor[] = {
    4u, USB_DESCRIPTOR_STRING, 0x09u, 0x04u
};

static const uint8_t USB_CODE usb_manufacturer_descriptor[] = {
    20u, USB_DESCRIPTOR_STRING,
    'o', 0u, 'h', 0u, '_', 0u, 'm', 0u, 'y', 0u,
    '_', 0u, 'u', 0u, 's', 0u, 'b', 0u
};

static const uint8_t USB_CODE usb_product_descriptor[] = {
    52u, USB_DESCRIPTOR_STRING,
    'U', 0u, 'S', 0u, 'B', 0u, ' ', 0u,
    'R', 0u, 'e', 0u, 'p', 0u, 'l', 0u, 'u', 0u, 'g', 0u, ' ', 0u,
    'D', 0u, 'o', 0u, 'n', 0u, 'g', 0u, 'l', 0u, 'e', 0u, ' ', 0u,
    'R', 0u, 'e', 0u, 'v', 0u, '.', 0u, 'C', 0u, '.', 0u, '1', 0u
};

static const uint8_t USB_CODE usb_interface_descriptor[] = {
    24u, USB_DESCRIPTOR_STRING,
    'C', 0u, 'o', 0u, 'n', 0u, 't', 0u, 'r', 0u, 'o', 0u,
    'l', 0u, ' ', 0u, 'C', 0u, 'D', 0u, 'C', 0u
};

const uint8_t *usb_descriptor_lookup(
    uint8_t descriptor_type,
    uint8_t descriptor_index,
    uint16_t *length
)
{
    if ((descriptor_type == USB_DESCRIPTOR_DEVICE) &&
        (descriptor_index == 0u)) {
        *length = sizeof(usb_device_descriptor);
        return usb_device_descriptor;
    }
    if ((descriptor_type == USB_DESCRIPTOR_CONFIGURATION) &&
        (descriptor_index == 0u)) {
        *length = sizeof(usb_configuration_descriptor);
        return usb_configuration_descriptor;
    }
    if (descriptor_type != USB_DESCRIPTOR_STRING) {
        *length = 0u;
        return 0;
    }

    switch (descriptor_index) {
    case 0u:
        *length = sizeof(usb_language_descriptor);
        return usb_language_descriptor;
    case 1u:
        *length = sizeof(usb_manufacturer_descriptor);
        return usb_manufacturer_descriptor;
    case 2u:
        *length = sizeof(usb_product_descriptor);
        return usb_product_descriptor;
    case 4u:
        *length = sizeof(usb_interface_descriptor);
        return usb_interface_descriptor;
    default:
        *length = 0u;
        return 0;
    }
}

static uint8_t hex_digit(uint8_t nibble)
{
    nibble &= 0x0fu;
    if (nibble < 10u) {
        return (uint8_t)('0' + nibble);
    }
    return (uint8_t)('A' + nibble - 10u);
}

uint8_t usb_build_serial_descriptor(uint8_t *destination, const uint8_t chip_id[5])
{
    static const char prefix[] = "RVC1-";
    uint8_t output_index;
    uint8_t index;
    uint8_t byte;

    destination[0] = USB_SERIAL_DESCRIPTOR_LENGTH;
    destination[1] = USB_DESCRIPTOR_STRING;
    output_index = 2u;

    for (index = 0u; index < 5u; ++index) {
        destination[output_index++] = (uint8_t)prefix[index];
        destination[output_index++] = 0u;
    }
    for (index = 0u; index < 5u; ++index) {
        byte = chip_id[index];
        destination[output_index++] = hex_digit((uint8_t)(byte >> 4));
        destination[output_index++] = 0u;
        destination[output_index++] = hex_digit(byte);
        destination[output_index++] = 0u;
    }
    return output_index;
}
