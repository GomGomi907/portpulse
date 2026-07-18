# USB Replug Dongle Rev.C.1 — CH552 firmware

This is the production-board firmware target for the cost-optimized Rev.C.1
hardware. The earlier `firmware/usb_replug_fw` directory is the superseded
STM32F042 Rev.C scaffold and must not be flashed to Rev.C.1.

## Implemented behavior

- CH552 USB Full-Speed CDC ACM command interface
- Windows inbox `usbser.sys` compatible device-class descriptors
- Stable chip-ID-derived serial number: `RVC1-XXXXXXXXXX`
- Reset-safe DUT state: VBUS off and D+/D- disconnected
- Active-low Rev.C.1 GPIO mapping
- Fault interlock and automatic safe-off after a fault on power-up
- Commands:
  - `on` / `on 1`
  - `off` / `off 1`
  - `cycle [100..30000]`
  - `cycle 1 [100..30000]`
  - `status [1]`
  - `fault`
  - `version`
  - `help`

The ON sequence connects D+/D- first, waits 20 ms, enables DUT VBUS, waits
100 ms, then checks the over-current fault again. OFF disconnects D+/D-
before removing VBUS.

## Pin contract

| CH552 pin | Net | Logical behavior |
|---|---|---|
| P1.0 / pin 7 | `TARGET_PWR_EN_N` | Low = DUT VBUS on |
| P1.1 / pin 8 | `TARGET_DATA_CMD` | Low = connected; open-drain release = disconnected |
| P3.1 / pin 9 | NC | High-impedance after startup; no CH334U reset connection |
| P3.0 / pin 10 | `TARGET_FAULT_N` | 5 V-domain input; low = over-current fault |
| P3.6 / pin 14 | `MCU_USB_DP` | USB D+ |
| P3.7 / pin 15 | `MCU_USB_DM` | USB D- |

CH334U `RESET#/CDP` is deliberately not driven by the MCU. The hub uses its
internal POR and pull-up as WCH recommends; TP8 remains available to assert a
manual active-low reset during bring-up.

Rev.C.1 powers CH552 `VCC` from `VBUS_IN` (nominal 5 V). That is a mandatory
precondition for the selected 24 MHz system clock; the build fails if
`CH552_VCC_MV` is configured below 4400.

## Build and tests

Prerequisites:

- SDCC 4.6.0 with MCS-51 support
- Python 3
- GCC for portable host-side C tests

Run:

```powershell
cd D:\oh_my_usb\firmware\usb_replug_ch552
py build.py all
```

Outputs:

```text
build/sdcc/usb_replug_ch552.bin
build/sdcc/usb_replug_ch552.hex
build/sdcc/usb_replug_ch552.ihx
build/sdcc/usb_replug_ch552.map
build/sdcc/usb_replug_ch552.mem
```

The build enforces the CH552 application limits: code below `0x3800` because
the upper 2 KiB is the factory bootloader, and application XRAM from `0x0100`
through `0x03ff`. USB DMA buffers occupy the hardware-sized fixed regions
`0x0000..0x003f` (EP0), `0x0040..0x007f` (EP1 TX), and
`0x0080..0x00ff` (EP2 RX/TX).

The build script also works around an SDCC 4.6.0 Windows packaging issue where
the private preprocessor backend can otherwise be confused with MinGW's
`cc1.exe`.

## USB identifiers

The default engineering VID/PID is `1A86:5722`, matching the host CLI defaults.
It is not an ownership grant. Before distribution, replace it at build time
with manufacturer-owned or licensed identifiers:

```powershell
# Add -DUSB_REPLUG_VID=... and -DUSB_REPLUG_PID=... to the SDCC flags.
```

## Programming

J3 exposes VBUS, GND, RST, P1.4, P1.6, and P1.7 for CH552 ISP/programming.
Program `build/sdcc/usb_replug_ch552.bin` with a CH55x-compatible WCH ISP tool.
Do not erase or overwrite the factory bootloader region at `0x3800..0x3fff`.

## Physical validation still required

Offline tests validate the command state machine, GPIO ordering contract,
descriptor structure, toolchain link, and memory limits. Final acceptance
requires assembled hardware for USB enumeration, voltage-level checks, fault
injection, and repeated DUT replug testing.
