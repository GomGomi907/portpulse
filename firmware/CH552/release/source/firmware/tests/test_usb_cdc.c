#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ch552_regs.h"
#include "usb_cdc.h"
#include "usb_descriptors.h"

volatile uint8_t P1;
volatile uint8_t P1_MOD_OC;
volatile uint8_t P1_DIR_PU;
volatile uint8_t P3_MOD_OC;
volatile uint8_t P3_DIR_PU;
volatile uint8_t SAFE_MOD;
volatile uint8_t P3;
volatile uint8_t GLOBAL_CFG;
volatile uint8_t CLOCK_CFG;
volatile uint8_t TCON;
volatile uint8_t TMOD;
volatile uint8_t TL0;
volatile uint8_t TH0;
volatile uint8_t T2MOD;
volatile uint8_t UDEV_CTRL;
volatile uint8_t UEP1_CTRL;
volatile uint8_t UEP1_T_LEN;
volatile uint8_t UEP2_CTRL;
volatile uint8_t UEP2_T_LEN;
volatile uint8_t USB_INT_FG;
volatile uint8_t USB_INT_ST;
volatile uint8_t USB_MIS_ST;
volatile uint8_t USB_RX_LEN;
volatile uint8_t UEP0_CTRL;
volatile uint8_t UEP0_T_LEN;
volatile uint8_t USB_INT_EN;
volatile uint8_t USB_CTRL;
volatile uint8_t USB_DEV_AD;
volatile uint8_t UEP2_DMA_L;
volatile uint8_t UEP2_DMA_H;
volatile uint8_t IE_EX;
volatile uint8_t UEP4_1_MOD;
volatile uint8_t UEP2_3_MOD;
volatile uint8_t UEP0_DMA_L;
volatile uint8_t UEP0_DMA_H;
volatile uint8_t UEP1_DMA_L;
volatile uint8_t UEP1_DMA_H;
volatile uint8_t P1_0;
volatile uint8_t P1_1;
volatile uint8_t P3_0;
volatile uint8_t P3_1;
volatile uint8_t TR0;
volatile uint8_t TF0;
volatile uint8_t EA;
volatile uint8_t IE_USB;
volatile uint8_t UIF_BUS_RST_BIT;
volatile uint8_t UIF_TRANSFER_BIT;
volatile uint8_t UIF_SUSPEND_BIT;
volatile uint8_t UIF_FIFO_OV_BIT;

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

static void transfer(uint8_t token_and_endpoint, uint8_t receive_length)
{
    USB_INT_ST = token_and_endpoint | bUIS_TOG_OK;
    USB_RX_LEN = receive_length;
    USB_INT_FG |= UIF_TRANSFER;
    usb_cdc_isr();
}

static void setup_request(
    uint8_t request_type,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint16_t length
)
{
    usb_ep0_buffer[0] = request_type;
    usb_ep0_buffer[1] = request;
    usb_ep0_buffer[2] = (uint8_t)value;
    usb_ep0_buffer[3] = (uint8_t)(value >> 8);
    usb_ep0_buffer[4] = (uint8_t)index;
    usb_ep0_buffer[5] = (uint8_t)(index >> 8);
    usb_ep0_buffer[6] = (uint8_t)length;
    usb_ep0_buffer[7] = (uint8_t)(length >> 8);
    transfer(UIS_TOKEN_SETUP, 8u);
}

static void configure_device(void)
{
    setup_request(0x00u, 0x09u, 1u, 0u, 0u);
    expect_true(usb_cdc_configured(), "SET_CONFIGURATION");
    transfer(UIS_TOKEN_IN, 0u);
}

static void test_unconfigured_bulk_out_is_rejected(void)
{
    const uint8_t chip_id[5] = {1u, 2u, 3u, 4u, 5u};

    usb_cdc_init(chip_id);
    expect_true(!usb_cdc_configured(), "device starts unconfigured");
    expect_true((UEP2_CTRL & MASK_UEP_R_RES) == UEP_R_RES_NAK,
        "unconfigured EP2 OUT must NAK");

    usb_ep2_buffer[0] = 'o';
    usb_ep2_buffer[1] = 'n';
    usb_ep2_buffer[2] = '\n';
    transfer((uint8_t)(UIS_TOKEN_OUT | 2u), 3u);
    expect_true(usb_cdc_available() == 0u,
        "unconfigured bulk OUT must not reach command parser");
    expect_true((UEP2_CTRL & MASK_UEP_R_RES) == UEP_R_RES_NAK,
        "unconfigured EP2 OUT remains NAK");
}

static void test_init_and_device_descriptor(void)
{
    const uint8_t chip_id[5] = {1u, 2u, 3u, 4u, 5u};

    usb_cdc_init(chip_id);
    expect_true((USB_CTRL & (bUC_DEV_PU_EN | bUC_DMA_EN)) ==
        (bUC_DEV_PU_EN | bUC_DMA_EN), "USB enable");
    expect_true(UEP4_1_MOD == bUEP1_TX_EN, "EP1 mode");
    expect_true(UEP2_3_MOD == (bUEP2_RX_EN | bUEP2_TX_EN), "EP2 mode");

    setup_request(0x80u, 0x06u, 0x0100u, 0u, 18u);
    expect_true(UEP0_T_LEN == 8u, "device descriptor first packet");
    expect_true(memcmp(usb_ep0_buffer, usb_device_descriptor, 8u) == 0,
        "device descriptor first bytes");
    transfer(UIS_TOKEN_IN, 0u);
    expect_true(UEP0_T_LEN == 8u, "device descriptor second packet");
    expect_true(memcmp(usb_ep0_buffer, usb_device_descriptor + 8, 8u) == 0,
        "device descriptor second bytes");
    transfer(UIS_TOKEN_IN, 0u);
    expect_true(UEP0_T_LEN == 2u, "device descriptor final packet");
    expect_true(memcmp(usb_ep0_buffer, usb_device_descriptor + 16, 2u) == 0,
        "device descriptor final bytes");
    transfer(UIS_TOKEN_IN, 0u);
    expect_true((UEP0_CTRL & MASK_UEP_T_RES) == UEP_T_RES_NAK,
        "descriptor status wait");

    setup_request(0x80u, 0x06u, 0x0101u, 0u, 18u);
    expect_true((UEP0_CTRL & MASK_UEP_T_RES) == UEP_T_RES_STALL,
        "reject invalid device descriptor index");
    setup_request(0x80u, 0x06u, 0x0301u, 0u, 20u);
    expect_true((UEP0_CTRL & MASK_UEP_T_RES) == UEP_T_RES_STALL,
        "reject invalid string LANGID");
}

static void test_address_and_line_coding(void)
{
    static const uint8_t new_line_coding[7] = {
        0x00u, 0x4Bu, 0x00u, 0x00u, 0u, 0u, 8u
    };

    setup_request(0x00u, 0x05u, 7u, 0u, 0u);
    expect_true(USB_DEV_AD == 0u, "address must be deferred");
    transfer(UIS_TOKEN_IN, 0u);
    expect_true(USB_DEV_AD == 7u, "address applied after status IN");

    setup_request(0x21u, 0x20u, 0u, 0u, 7u);
    memcpy(usb_ep0_buffer, new_line_coding, sizeof(new_line_coding));
    transfer(UIS_TOKEN_OUT, 7u);
    expect_true(UEP0_T_LEN == 0u &&
        (UEP0_CTRL & MASK_UEP_T_RES) == UEP_T_RES_ACK,
        "SET_LINE_CODING status");
    transfer(UIS_TOKEN_IN, 0u);

    setup_request(0xA1u, 0x21u, 0u, 0u, 7u);
    expect_true(UEP0_T_LEN == 7u, "GET_LINE_CODING length");
    expect_true(memcmp(usb_ep0_buffer, new_line_coding, 7u) == 0,
        "GET_LINE_CODING contents");
}

static void test_endpoint_halt_and_bounded_tx(void)
{
    static const uint8_t payload[] = {'O', 'K'};

    configure_device();
    setup_request(0x02u, 0x03u, 0u, 0x82u, 0u);
    expect_true((UEP2_CTRL & MASK_UEP_T_RES) == UEP_T_RES_STALL,
        "SET_FEATURE halt EP2 IN");
    transfer(UIS_TOKEN_IN, 0u);

    setup_request(0x82u, 0x00u, 0u, 0x82u, 2u);
    expect_true(usb_ep0_buffer[0] == 1u, "GET_STATUS reports halt");
    expect_true(!usb_cdc_write(payload, sizeof(payload)),
        "write rejected while halted");

    setup_request(0x02u, 0x01u, 0u, 0x82u, 0u);
    expect_true((UEP2_CTRL & MASK_UEP_T_RES) == UEP_T_RES_NAK,
        "CLEAR_FEATURE resets EP2 IN");
    transfer(UIS_TOKEN_IN, 0u);
    expect_true(usb_cdc_write(payload, sizeof(payload)), "queue TX");
    expect_true(UEP2_T_LEN == sizeof(payload), "TX packet length");
    expect_true(!usb_cdc_flush(), "TX wait is bounded without host ACK");
    expect_true((UEP2_CTRL & MASK_UEP_T_RES) == UEP_T_RES_NAK,
        "timed-out TX returns to NAK");
}

static void test_bulk_out(void)
{
    usb_ep2_buffer[0] = 'o';
    usb_ep2_buffer[1] = 'n';
    usb_ep2_buffer[2] = '\n';
    transfer((uint8_t)(UIS_TOKEN_OUT | 2u), 3u);
    expect_true(usb_cdc_available() == 3u, "bulk OUT available");
    expect_true((UEP2_CTRL & MASK_UEP_R_RES) == UEP_R_RES_NAK,
        "bulk OUT backpressure");
    expect_true(usb_cdc_read() == 'o', "bulk OUT byte 0");
    expect_true(usb_cdc_read() == 'n', "bulk OUT byte 1");
    expect_true(usb_cdc_read() == '\n', "bulk OUT byte 2");
    expect_true((UEP2_CTRL & MASK_UEP_R_RES) == UEP_R_RES_ACK,
        "bulk OUT rearmed");
}

int main(void)
{
    test_unconfigured_bulk_out_is_rejected();
    test_init_and_device_descriptor();
    test_address_and_line_coding();
    test_endpoint_halt_and_bounded_tx();
    test_bulk_out();
    puts("test_usb_cdc: PASS");
    return 0;
}
