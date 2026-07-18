# USB Replug Dongle Rev.C.1 CH552 Firmware Bring-up

Date: 2026-07-17  
Project: `D:\oh_my_usb`  
Target: cost-optimized Rev.C.1 production hardware

> This document supersedes `FIRMWARE_BRINGUP_BRIEF_REV_C.md`, which describes
> the older STM32F042 Rev.C board.

## 1. Deliverable

The production firmware project is:

```text
firmware/usb_replug_ch552/
```

It builds a CH552 binary exposing a USB CDC ACM control channel. The PC sees:

```text
CH334U USB 2.0 hub
├─ CH552 CDC control device (always connected)
└─ DUT port (FSUSB42 data disconnect + TPS2552 VBUS disconnect)
```

## 2. Safety invariants

Before USB initialization and after every reset:

```text
TARGET_PWR_EN_N = high  -> DUT VBUS off
TARGET_DATA_CMD = open-drain release -> FSUSB42 /OE high, D+/D- disconnected
HUB_RESET_N     = un-driven -> CH334U internal POR/pull-up; TP8 manual low reset
TARGET_FAULT_N  = input -> TPS2552 FAULT and CH552 P3.0 share the 5 V domain
```

CH552 P3.1 is physically unconnected. This removes the unsafe pre-firmware
5 V quasi-bidirectional pull-up path into the CH334U 3.3 V reset input.
The FSUSB42 `/OE` network uses equal 5.1k parts (R6/R8/R9), keeping `/OE` below
3.3 V even during CH552 POR while preserving a valid disconnected-state high.

Command order:

- `on`: connect data, wait 20 ms, enable VBUS, wait 100 ms, recheck fault.
- `off`: disconnect data, disable VBUS.
- `cycle`: off, wait 100–30000 ms, on.
- An active fault rejects `on`; a fault after enabling power forces safe-off.

## 3. USB interface

- USB 2.0 Full-Speed CDC ACM
- Device class/subclass/protocol: `02/00/00`
- EP0 control: 8 bytes
- EP1 interrupt IN: 8 bytes
- EP2 bulk OUT/IN: 64 bytes
- Stable serial: `RVC1-` plus ten uppercase hexadecimal chip-ID digits
- Engineering VID/PID: `1A86:5722`
- CH552 `VCC`: `VBUS_IN`, nominal 5 V; required for the selected 24 MHz clock

Windows can bind the inbox `usbser.sys` driver based on the compatible class
IDs. Linux and macOS use their normal CDC ACM drivers.

## 4. Command protocol

ASCII commands end in CR or LF. Responses end in CRLF.

```text
on
off
cycle
cycle 2000
on 1
off 1
cycle 1 2000
status
fault
version
help
```

Examples:

```text
OK
ERR invalid_command
ERR invalid_arg
ERR fault_active
STATE off fault=0 version=0.2.1-revc1
FAULT 0
VERSION 0.2.1-revc1
```

## 5. Build

```powershell
cd D:\oh_my_usb\firmware\usb_replug_ch552
py build.py clean
py build.py all
```

The build runs portable C unit tests before compiling with SDCC. The linked
image must remain below `0x3800`; the factory bootloader occupies the top 2 KiB.

## 6. Host CLI

```powershell
cd D:\oh_my_usb\host\usb-replug-cli
py -m pip install pyserial
py -m usb_replug ports
py -m usb_replug status
py -m usb_replug cycle --delay-ms 2000
```

The CLI automatically finds `1A86:5722`, supports `--serial` when several
dongles are attached, and accepts `--port` for explicit selection.

## 7. PCBA bring-up sequence

1. Inspect assembly polarity/orientation and shorts before connecting USB.
2. Verify `VBUS_IN`, `3V3`, and GND.
3. With blank MCU, verify DUT VBUS is off and FSUSB42 is disabled.
4. Program the firmware through J3 without overwriting the factory bootloader.
5. Verify CH334U hub enumeration.
6. Verify CH552 CDC enumeration and stable serial number.
7. Run `status`, `on`, `off`, and `cycle 2000`.
8. Measure TP5/TP6/TP7/TP8 and DUT VBUS against the command state.
9. Inject a current-limit fault and verify safe-off plus `ERR fault_active`.
10. Run 100 cycles against each representative DUT and collect OS logs.

## 8. Acceptance gates

Offline:

- command core unit tests pass
- descriptor tests pass
- host CLI tests pass
- SDCC build/link passes
- code and XRAM stay within enforced limits

On hardware:

- enumeration on Windows/Linux/macOS
- no 5 V drive into 3.3 V-only nets
- default safe-off at power-up/reset
- over-current safe-off
- 100-cycle reliability test
- no MCU/hub hang and no persistent OS USB errors
