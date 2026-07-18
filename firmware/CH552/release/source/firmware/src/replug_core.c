#include "replug_core.h"

#ifdef __SDCC_mcs51
#define REPLUG_XDATA __xdata
#else
#define REPLUG_XDATA
#endif

#define REPLUG_LINE_CAPACITY 32u
#define REPLUG_MAX_ARGS 3u
#define REPLUG_ON_SETTLE_MS 20u
#define REPLUG_POWER_SETTLE_MS 100u
#define REPLUG_DELAY_SLICE_MS 10u

static REPLUG_XDATA char line_buffer[REPLUG_LINE_CAPACITY];
static uint8_t line_length;
static uint8_t line_ready;
static uint8_t line_overflow;
static uint8_t target_is_on;

static uint8_t is_space(char value)
{
    return (uint8_t)((value == ' ') || (value == '\t'));
}

static uint8_t strings_equal(const char *left, const char *right)
{
    while ((*left != '\0') && (*right != '\0')) {
        if (*left != *right) {
            return 0u;
        }
        ++left;
        ++right;
    }
    return (uint8_t)((*left == '\0') && (*right == '\0'));
}

static uint8_t parse_u16(const char *text, uint16_t *value)
{
    uint32_t parsed;
    uint8_t digits;

    parsed = 0u;
    digits = 0u;
    while (*text != '\0') {
        if ((*text < '0') || (*text > '9')) {
            return 0u;
        }
        parsed = (parsed * 10u) + (uint8_t)(*text - '0');
        if (parsed > 65535u) {
            return 0u;
        }
        ++digits;
        ++text;
    }
    if (digits == 0u) {
        return 0u;
    }
    *value = (uint16_t)parsed;
    return 1u;
}

static uint8_t valid_port_arg(const char *text)
{
    return strings_equal(text, "1");
}

static void target_off(void)
{
    board_target_data_set(0u);
    board_target_power_set(0u);
    target_is_on = 0u;
}

static uint8_t target_on(void)
{
    if (board_fault_active()) {
        return 0u;
    }

    /* Rev.C.1 contract: connect D+/D- before applying DUT VBUS. */
    board_target_data_set(1u);
    board_delay_ms(REPLUG_ON_SETTLE_MS);
    if (board_fault_active()) {
        target_off();
        return 0u;
    }
    board_target_power_set(1u);
    board_delay_ms(REPLUG_POWER_SETTLE_MS);

    if (board_fault_active()) {
        target_off();
        return 0u;
    }

    target_is_on = 1u;
    return 1u;
}

static void send_status(void)
{
    if (target_is_on) {
        if (board_fault_active()) {
            transport_send_line("STATE on fault=1 version=" REPLUG_VERSION);
        } else {
            transport_send_line("STATE on fault=0 version=" REPLUG_VERSION);
        }
    } else if (board_fault_active()) {
        transport_send_line("STATE off fault=1 version=" REPLUG_VERSION);
    } else {
        transport_send_line("STATE off fault=0 version=" REPLUG_VERSION);
    }
}

static uint8_t delay_cycle(uint16_t milliseconds)
{
    uint16_t remaining;
    uint16_t slice;

    remaining = milliseconds;
    while (remaining != 0u) {
        slice = remaining;
        if (slice > REPLUG_DELAY_SLICE_MS) {
            slice = REPLUG_DELAY_SLICE_MS;
        }
        board_delay_ms(slice);
        if (board_fault_active()) {
            return 0u;
        }
        remaining = (uint16_t)(remaining - slice);
    }
    return 1u;
}

static uint8_t tokenize(char **args)
{
    char *cursor;
    uint8_t count;

    cursor = line_buffer;
    count = 0u;
    while (*cursor != '\0') {
        while (is_space(*cursor)) {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        if (count == REPLUG_MAX_ARGS) {
            return (uint8_t)(REPLUG_MAX_ARGS + 1u);
        }
        args[count++] = cursor;
        while ((*cursor != '\0') && !is_space(*cursor)) {
            ++cursor;
        }
        if (*cursor != '\0') {
            *cursor++ = '\0';
        }
    }
    return count;
}

static void handle_on(uint8_t argc, char **args)
{
    if ((argc > 2u) || ((argc == 2u) && !valid_port_arg(args[1]))) {
        transport_send_line("ERR invalid_arg");
        return;
    }
    if (!target_on()) {
        transport_send_line("ERR fault_active");
        return;
    }
    transport_send_line("OK");
}

static void handle_off(uint8_t argc, char **args)
{
    if ((argc > 2u) || ((argc == 2u) && !valid_port_arg(args[1]))) {
        transport_send_line("ERR invalid_arg");
        return;
    }
    target_off();
    transport_send_line("OK");
}

static void handle_cycle(uint8_t argc, char **args)
{
    uint16_t delay_ms;
    const char *delay_arg;

    delay_ms = REPLUG_DEFAULT_CYCLE_MS;
    delay_arg = 0;

    if (argc == 2u) {
        delay_arg = args[1];
    } else if (argc == 3u) {
        if (!valid_port_arg(args[1])) {
            transport_send_line("ERR invalid_arg");
            return;
        }
        delay_arg = args[2];
    } else if (argc != 1u) {
        transport_send_line("ERR invalid_arg");
        return;
    }

    if (delay_arg != 0) {
        if (!parse_u16(delay_arg, &delay_ms) ||
            (delay_ms < REPLUG_MIN_CYCLE_MS) ||
            (delay_ms > REPLUG_MAX_CYCLE_MS)) {
            transport_send_line("ERR invalid_arg");
            return;
        }
    }

    target_off();
    if (!delay_cycle(delay_ms)) {
        transport_send_line("ERR fault_active");
        return;
    }
    if (!target_on()) {
        transport_send_line("ERR fault_active");
        return;
    }
    transport_send_line("OK");
}

static void process_line(void)
{
    char *args[REPLUG_MAX_ARGS];
    uint8_t argc;

    if (line_overflow) {
        transport_send_line("ERR invalid_arg");
        return;
    }

    argc = tokenize(args);
    if (argc == 0u) {
        return;
    }
    if (argc > REPLUG_MAX_ARGS) {
        transport_send_line("ERR invalid_arg");
    } else if (strings_equal(args[0], "on")) {
        handle_on(argc, args);
    } else if (strings_equal(args[0], "off")) {
        handle_off(argc, args);
    } else if (strings_equal(args[0], "cycle")) {
        handle_cycle(argc, args);
    } else if (strings_equal(args[0], "status")) {
        if ((argc == 1u) || ((argc == 2u) && valid_port_arg(args[1]))) {
            send_status();
        } else {
            transport_send_line("ERR invalid_arg");
        }
    } else if (strings_equal(args[0], "fault") && (argc == 1u)) {
        transport_send_line(board_fault_active() ? "FAULT 1" : "FAULT 0");
    } else if (strings_equal(args[0], "version") && (argc == 1u)) {
        transport_send_line("VERSION " REPLUG_VERSION);
    } else if (strings_equal(args[0], "help") && (argc == 1u)) {
        transport_send_line("COMMANDS on off cycle [100..30000] status fault version help");
    } else {
        transport_send_line("ERR invalid_command");
    }
}

void replug_core_init(void)
{
    line_length = 0u;
    line_ready = 0u;
    line_overflow = 0u;
    target_is_on = 0u;
    board_target_data_set(0u);
    board_target_power_set(0u);
}

void replug_core_feed_byte(uint8_t byte)
{
    char character;

    if (line_ready) {
        return;
    }

    character = (char)byte;
    if ((character == '\r') || (character == '\n')) {
        if ((line_length != 0u) || line_overflow) {
            line_buffer[line_length] = '\0';
            line_ready = 1u;
        }
        return;
    }

    if ((character >= 'A') && (character <= 'Z')) {
        character = (char)(character + ('a' - 'A'));
    }
    if (((character < 0x20) && (character != '\t')) || (character > 0x7e)) {
        line_overflow = 1u;
        return;
    }
    if (line_length >= (REPLUG_LINE_CAPACITY - 1u)) {
        line_overflow = 1u;
        return;
    }
    line_buffer[line_length++] = character;
}

void replug_core_poll(void)
{
    if (target_is_on && board_fault_active()) {
        target_off();
    }

    if (!line_ready) {
        return;
    }

    process_line();
    line_length = 0u;
    line_ready = 0u;
    line_overflow = 0u;
}

uint8_t replug_core_target_is_on(void)
{
    return target_is_on;
}
