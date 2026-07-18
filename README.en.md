# PortPulse

This is the supplementary English documentation. The primary project
documentation is [README.md](README.md) in Korean.

## What it is

PortPulse is a USB 2.0 replug dongle placed between a PC and a USB device under
test (DUT).

It keeps the control MCU connected to the PC while software disconnects and
restores the DUT's VBUS and D+/D−. This reproduces the effect of physically
unplugging and reconnecting a USB device during automated testing.

## How to use it

1. Connect the USB-C upstream port to the PC.
2. Connect the DUT to the USB-A downstream port.
3. Find PortPulse's COM port in the operating system.
4. Send commands with the native `portpulse` program.

```text
portpulse --port COM12 status
portpulse --port COM12 on
portpulse --port COM12 off
portpulse --port COM12 --delay-ms 2000 cycle
```

Commands:

- `on`: connect the DUT
- `off`: disconnect the DUT
- `cycle`: disconnect, wait, and reconnect the DUT
- `status`: read the current state
- `fault`: read the power-switch fault state
- `version`: read the firmware version

Rev.C.1 supports one DUT port. When multiple PortPulse devices are connected,
select each device by its COM port.

Hardware, firmware, and host software are released under the MIT License unless
a bundled third-party notice says otherwise.
