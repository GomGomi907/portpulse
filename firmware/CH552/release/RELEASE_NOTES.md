# USB Replug Dongle Rev.C.1 Firmware 0.2.1

Release date: 2026-07-18  
Target MCU: WCH CH552T  
Target PCB: USB Replug Dongle Rev.C.1 with CH334U hub

## Release status

**Offline firmware release: PASS.** The former CH552 reset-state blocker is
removed in hardware: P3.1 is NC, CH334U RESET#/CDP is only connected to manual
TP8, and TPS2552 FAULT/P3.0 now share the 5 V domain. The equal 5.1 kΩ
R6/R8/R9 network keeps FSUSB42 `/OE` reset-safe.

Physical PCBA bring-up remains mandatory before product acceptance. It includes
rail measurements, hub/CDC enumeration, FSUSB42 and TPS2552 behavior, fault
injection, and 100-cycle representative-DUT testing.

## Behavior

- Safe boot: DUT VBUS off and D+/D- disconnected
- ON: connect data, wait 20 ms, enable VBUS, wait 100 ms, verify fault
- OFF: disconnect data, then disable VBUS
- Cycle range: 100–30000 ms; default 2000 ms
- Active-low over-current interlock with automatic safe-off
- CDC commands: `on`, `off`, `cycle`, `status`, `fault`, `version`, `help`
- Stable USB serial derived from the CH552 unique ID

## USB identity

Engineering default VID:PID is `1A86:5722`. Replace it with
manufacturer-owned or licensed identifiers before commercial distribution.

## Supersession

This release supersedes the blocked `0.2.0-revc1` archive. Do not flash the
older archive to production Rev.C.1 PCBAs.
