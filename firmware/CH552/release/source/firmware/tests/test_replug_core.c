#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "replug_core.h"

static char event_log[512];
static size_t event_length;
static char response[160];
static uint8_t fake_fault;
static uint32_t delayed_ms;
static uint32_t fault_after_ms;

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

static void append_event(char event)
{
    if (event_length + 1u >= sizeof(event_log)) {
        fail("event log overflow");
    }
    event_log[event_length++] = event;
    event_log[event_length] = '\0';
}

void board_target_power_set(uint8_t on)
{
    append_event(on ? 'P' : 'p');
}

void board_target_data_set(uint8_t connected)
{
    append_event(connected ? 'D' : 'd');
}

uint8_t board_fault_active(void)
{
    return fake_fault;
}

void board_delay_ms(uint16_t milliseconds)
{
    delayed_ms += milliseconds;
    if ((fault_after_ms != 0u) && (delayed_ms >= fault_after_ms)) {
        fake_fault = 1u;
    }
}

void transport_send_line(const char *text)
{
    size_t length;

    length = strlen(text);
    if (length >= sizeof(response)) {
        fail("response overflow");
    }
    memcpy(response, text, length + 1u);
}

static void reset_fixture(void)
{
    event_length = 0u;
    event_log[0] = '\0';
    response[0] = '\0';
    fake_fault = 0u;
    delayed_ms = 0u;
    fault_after_ms = 0u;
    replug_core_init();
    event_length = 0u;
    event_log[0] = '\0';
}

static void command(const char *text)
{
    while (*text != '\0') {
        replug_core_feed_byte((uint8_t)*text++);
    }
    replug_core_feed_byte('\n');
    replug_core_poll();
}

static void test_safe_initial_state(void)
{
    event_length = 0u;
    event_log[0] = '\0';
    replug_core_init();
    expect_true(strcmp(event_log, "dp") == 0, "init must disconnect data and disable power");
    expect_true(replug_core_target_is_on() == 0u, "init state must be off");
}

static void test_on_off_order(void)
{
    reset_fixture();
    command("on");
    expect_true(strcmp(response, "OK") == 0, "on response");
    expect_true(strcmp(event_log, "DP") == 0, "on must connect data before power");
    expect_true(delayed_ms == 120u, "on settling delay");
    expect_true(replug_core_target_is_on() == 1u, "on state");

    event_length = 0u;
    event_log[0] = '\0';
    command("off");
    expect_true(strcmp(response, "OK") == 0, "off response");
    expect_true(strcmp(event_log, "dp") == 0, "off must disconnect data before power");
    expect_true(replug_core_target_is_on() == 0u, "off state");
}

static void test_fault_interlock(void)
{
    reset_fixture();
    fake_fault = 1u;
    command("on");
    expect_true(strcmp(response, "ERR fault_active") == 0, "active fault must reject on");
    expect_true(event_length == 0u, "faulted on must not switch outputs");

    reset_fixture();
    fault_after_ms = 20u;
    command("on");
    expect_true(strcmp(response, "ERR fault_active") == 0,
        "fault during data settle must reject on");
    expect_true(strcmp(event_log, "Ddp") == 0,
        "fault during data settle must not enable power");
    expect_true(delayed_ms == 20u,
        "fault during data settle must abort before power settle");
    expect_true(replug_core_target_is_on() == 0u,
        "fault during data settle must leave target off");

    reset_fixture();
    command("on");
    fake_fault = 1u;
    event_length = 0u;
    event_log[0] = '\0';
    replug_core_poll();
    expect_true(strcmp(event_log, "dp") == 0, "runtime fault must force safe-off");
    expect_true(replug_core_target_is_on() == 0u, "runtime fault state");
}

static void test_cycle_and_ranges(void)
{
    reset_fixture();
    command("cycle 250");
    expect_true(strcmp(response, "OK") == 0, "cycle response");
    expect_true(strcmp(event_log, "dpDP") == 0, "cycle output order");
    expect_true(delayed_ms == 370u, "cycle delay plus on settling");

    response[0] = '\0';
    command("cycle 99");
    expect_true(strcmp(response, "ERR invalid_arg") == 0, "cycle low bound");
    command("cycle 1 30000");
    expect_true(strcmp(response, "OK") == 0, "port-form cycle");

    reset_fixture();
    fault_after_ms = 30u;
    command("cycle 250");
    expect_true(strcmp(response, "ERR fault_active") == 0, "fault during cycle delay");
    expect_true(strcmp(event_log, "dp") == 0, "faulted cycle must not reconnect");
    expect_true(delayed_ms == 30u, "cycle fault must abort promptly");
}

static void test_protocol_surface(void)
{
    reset_fixture();
    command("STATUS 1");
    expect_true(strstr(response, "STATE off fault=0 version=") == response, "status format");
    command("fault");
    expect_true(strcmp(response, "FAULT 0") == 0, "fault format");
    command("version");
    expect_true(strcmp(response, "VERSION " REPLUG_VERSION) == 0, "version format");
    command("help");
    expect_true(strstr(response, "COMMANDS ") == response, "help format");
    command("on 2");
    expect_true(strcmp(response, "ERR invalid_arg") == 0, "reject invalid port");
    command("unknown");
    expect_true(strcmp(response, "ERR invalid_command") == 0, "reject unknown command");
}

static void test_line_handling(void)
{
    unsigned int index;

    reset_fixture();
    command("\t ON \t");
    expect_true(strcmp(response, "OK") == 0, "whitespace and case normalization");

    reset_fixture();
    for (index = 0u; index < 80u; ++index) {
        replug_core_feed_byte('x');
    }
    replug_core_feed_byte('\n');
    replug_core_poll();
    expect_true(strcmp(response, "ERR invalid_arg") == 0, "overflow handling");
}

int main(void)
{
    test_safe_initial_state();
    test_on_off_order();
    test_fault_interlock();
    test_cycle_and_ranges();
    test_protocol_surface();
    test_line_handling();
    puts("test_replug_core: PASS");
    return 0;
}
