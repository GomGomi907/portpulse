#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usb_descriptors.h"

static void fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
}

static void expect_true(int condition, const char *message)
{
    if (!condition) {
        fail(message);
    }
}

static void test_device_descriptor(void)
{
    expect_true(sizeof(usb_device_descriptor) == 18u, "device length");
    expect_true(usb_device_descriptor[0] == 18u, "device bLength");
    expect_true(usb_device_descriptor[1] == USB_DESCRIPTOR_DEVICE, "device type");
    expect_true(usb_device_descriptor[4] == 0x02u, "device class CDC");
    expect_true(usb_device_descriptor[5] == 0x00u, "device subclass unused");
    expect_true(usb_device_descriptor[6] == 0x00u, "device protocol unused");
    expect_true(usb_device_descriptor[7] == 8u, "EP0 packet size");
    expect_true(
        ((uint16_t)usb_device_descriptor[8] |
         ((uint16_t)usb_device_descriptor[9] << 8)) == USB_REPLUG_VID,
        "VID"
    );
    expect_true(
        ((uint16_t)usb_device_descriptor[10] |
         ((uint16_t)usb_device_descriptor[11] << 8)) == USB_REPLUG_PID,
        "PID"
    );
}

static void test_configuration_walk(void)
{
    size_t offset;
    unsigned int interfaces;
    unsigned int endpoints;
    unsigned int in_endpoints;
    unsigned int out_endpoints;

    expect_true(sizeof(usb_configuration_descriptor) == USB_CDC_CONFIGURATION_LENGTH, "config array length");
    expect_true(usb_configuration_descriptor[0] == 9u, "config bLength");
    expect_true(usb_configuration_descriptor[1] == USB_DESCRIPTOR_CONFIGURATION, "config type");
    expect_true(
        ((uint16_t)usb_configuration_descriptor[2] |
         ((uint16_t)usb_configuration_descriptor[3] << 8)) ==
            USB_CDC_CONFIGURATION_LENGTH,
        "config wTotalLength"
    );
    expect_true(usb_configuration_descriptor[4] == 2u, "two interfaces");

    offset = 0u;
    interfaces = 0u;
    endpoints = 0u;
    in_endpoints = 0u;
    out_endpoints = 0u;
    while (offset < sizeof(usb_configuration_descriptor)) {
        uint8_t length;
        uint8_t type;

        length = usb_configuration_descriptor[offset];
        expect_true(length >= 2u, "descriptor length must be nonzero");
        expect_true(offset + length <= sizeof(usb_configuration_descriptor), "descriptor bounds");
        type = usb_configuration_descriptor[offset + 1u];
        if (type == 0x04u) {
            ++interfaces;
        } else if (type == 0x05u) {
            ++endpoints;
            if (usb_configuration_descriptor[offset + 2u] & 0x80u) {
                ++in_endpoints;
            } else {
                ++out_endpoints;
            }
        }
        offset += length;
    }
    expect_true(offset == sizeof(usb_configuration_descriptor), "descriptor walk exact");
    expect_true(interfaces == 2u, "interface descriptor count");
    expect_true(endpoints == 3u, "endpoint descriptor count");
    expect_true(in_endpoints == 2u && out_endpoints == 1u, "endpoint direction count");
}

static void test_lookup_and_serial(void)
{
    const uint8_t chip_id[5] = {0x01u, 0x23u, 0x45u, 0x67u, 0x89u};
    uint8_t serial[USB_SERIAL_DESCRIPTOR_LENGTH];
    uint16_t length;
    const uint8_t *descriptor;
    const char expected[] = "RVC1-0123456789";
    size_t index;

    descriptor = usb_descriptor_lookup(USB_DESCRIPTOR_DEVICE, 0u, &length);
    expect_true(descriptor == usb_device_descriptor && length == 18u, "device lookup");
    descriptor = usb_descriptor_lookup(USB_DESCRIPTOR_DEVICE, 1u, &length);
    expect_true(descriptor == NULL && length == 0u, "reject device descriptor index");
    descriptor = usb_descriptor_lookup(USB_DESCRIPTOR_CONFIGURATION, 1u, &length);
    expect_true(descriptor == NULL && length == 0u, "reject configuration descriptor index");
    descriptor = usb_descriptor_lookup(USB_DESCRIPTOR_STRING, 3u, &length);
    expect_true(descriptor == NULL && length == 0u, "serial is dynamic");
    expect_true(usb_build_serial_descriptor(serial, chip_id) == sizeof(serial), "serial length");
    expect_true(serial[0] == sizeof(serial) && serial[1] == USB_DESCRIPTOR_STRING, "serial header");
    for (index = 0u; index < strlen(expected); ++index) {
        expect_true(serial[2u + index * 2u] == (uint8_t)expected[index], "serial character");
        expect_true(serial[3u + index * 2u] == 0u, "serial UTF-16 high byte");
    }
}

int main(void)
{
    test_device_descriptor();
    test_configuration_walk();
    test_lookup_and_serial();
    puts("test_descriptors: PASS");
    return 0;
}
