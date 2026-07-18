#include <stdint.h>

#include "ch552_regs.h"
#include "replug_core.h"
#include "usb_cdc.h"

#ifndef CH552_VCC_MV
#define CH552_VCC_MV 5000
#endif

#if CH552_VCC_MV < 4400
#error "24 MHz CH552 clock requires VCC above 4.4 V"
#endif

#define TARGET_PWR_MASK 0x01u
#define TARGET_DATA_MASK 0x02u
#define TARGET_FAULT_MASK 0x01u
#define UNUSED_P3_1_MASK 0x02u

static void board_safe_gpio_init(void)
{
    /*
     * Set output latches high before changing direction. P1.0 becomes a
     * push-pull output for the 5 V power-enable net. P1.1 remains open-drain
     * with no internal pull-up so the external R6/R8/R9 network keeps the
     * 3.3 V FSUSB42 /OE input within its supply while data is disconnected.
     */
    P1 |= TARGET_PWR_MASK | TARGET_DATA_MASK;
    P1_DIR_PU &= (uint8_t)~TARGET_DATA_MASK;
    P1_MOD_OC |= TARGET_DATA_MASK;
    P1_MOD_OC &= (uint8_t)~TARGET_PWR_MASK;
    P1_DIR_PU |= TARGET_PWR_MASK;

    /*
     * P3.0 is a high-impedance input on the 5 V-domain FAULT_N net.
     * P3.1 is physically unconnected from CH334R RESET#/CDP and is also
     * placed in high-impedance/no-pull mode after startup.
     */
    P3 |= TARGET_FAULT_MASK | UNUSED_P3_1_MASK;
    P3_DIR_PU &= (uint8_t)~(TARGET_FAULT_MASK | UNUSED_P3_1_MASK);
    P3_MOD_OC &= (uint8_t)~(TARGET_FAULT_MASK | UNUSED_P3_1_MASK);
}

static void system_clock_init(void)
{
    SAFE_MOD = CH552_SAFE_UNLOCK_1;
    SAFE_MOD = CH552_SAFE_UNLOCK_2;
    CLOCK_CFG = (CLOCK_CFG & (uint8_t)~CH552_MASK_SYS_CK_SEL) |
        CH552_CLOCK_24MHZ;
    SAFE_MOD = 0u;
}

static void timer0_init(void)
{
    TR0 = 0;
    T2MOD &= (uint8_t)~bT0_CLK;
    TMOD = (TMOD & 0xF0u) | 0x01u;
    TF0 = 0;
}

static void read_chip_id(uint8_t chip_id[5])
{
    const __code uint8_t *id0;
    const __code uint8_t *id1;
    uint8_t index;

    id0 = (const __code uint8_t *)0x3FFAu;
    id1 = (const __code uint8_t *)0x3FFCu;
    chip_id[0] = *id0;
    for (index = 0u; index < 4u; ++index) {
        chip_id[(uint8_t)(index + 1u)] = id1[index];
    }
}

void board_target_power_set(uint8_t on)
{
    P1_0 = on ? 0 : 1;
}

void board_target_data_set(uint8_t connected)
{
    P1_1 = connected ? 0 : 1;
}

uint8_t board_fault_active(void)
{
    return P3_0 ? 0u : 1u;
}

void board_delay_ms(uint16_t milliseconds)
{
    while (milliseconds-- != 0u) {
        /* 24 MHz / 12 = 2 MHz; 2000 timer counts are exactly 1 ms. */
        TH0 = 0xF8u;
        TL0 = 0x30u;
        TF0 = 0;
        TR0 = 1;
        while (!TF0) {
            /* USB interrupts remain enabled during the delay. */
        }
        TR0 = 0;
        TF0 = 0;
    }
}

void transport_send_line(const char *text)
{
    uint8_t length;

    length = 0u;
    while (text[length] != '\0') {
        ++length;
    }
    if (!usb_cdc_write((const uint8_t *)text, length)) {
        return;
    }
    if (!usb_cdc_write((const uint8_t *)"\r\n", 2u)) {
        return;
    }
    (void)usb_cdc_flush();
}

void usb_interrupt(void) __interrupt(CH552_INT_NO_USB)
{
    usb_cdc_isr();
}

void main(void)
{
    uint8_t chip_id[5];

    board_safe_gpio_init();
    system_clock_init();
    timer0_init();
    replug_core_init();
    read_chip_id(chip_id);
    usb_cdc_init(chip_id);

    for (;;) {
        while (usb_cdc_available()) {
            replug_core_feed_byte(usb_cdc_read());
            replug_core_poll();
        }
        replug_core_poll();
    }
}
