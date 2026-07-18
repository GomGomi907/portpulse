#ifndef USB_REPLUG_USB_CDC_H
#define USB_REPLUG_USB_CDC_H

#include <stdint.h>

void usb_cdc_init(const uint8_t chip_id[5]);
void usb_cdc_isr(void);
uint8_t usb_cdc_configured(void);
uint8_t usb_cdc_available(void);
uint8_t usb_cdc_read(void);
uint8_t usb_cdc_write(const uint8_t *data, uint8_t length);
uint8_t usb_cdc_flush(void);

#ifdef USB_CDC_HOST_TEST
extern uint8_t usb_ep0_buffer[64];
extern uint8_t usb_ep1_buffer[64];
extern uint8_t usb_ep2_buffer[128];
#endif

#endif
