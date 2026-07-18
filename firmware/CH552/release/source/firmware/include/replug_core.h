#ifndef USB_REPLUG_CORE_H
#define USB_REPLUG_CORE_H

#include <stdint.h>

#define REPLUG_VERSION "0.2.1-revc1"
#define REPLUG_DEFAULT_CYCLE_MS 2000u
#define REPLUG_MIN_CYCLE_MS 100u
#define REPLUG_MAX_CYCLE_MS 30000u

void replug_core_init(void);
void replug_core_feed_byte(uint8_t byte);
void replug_core_poll(void);
uint8_t replug_core_target_is_on(void);

/*
 * Board/transport functions supplied by the target or the host-side tests.
 * All logical values are positive even though the Rev.C.1 GPIO nets are
 * active-low.
 */
void board_target_power_set(uint8_t on);
void board_target_data_set(uint8_t connected);
uint8_t board_fault_active(void);
void board_delay_ms(uint16_t milliseconds);
void transport_send_line(const char *text);

#endif
