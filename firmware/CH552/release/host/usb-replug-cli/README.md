# USB Replug Dongle Rev.C.1 Host CLI

CDC serial CLI for CH552 firmware `0.2.1-revc1`.

## Usage

Run from this directory:

```powershell
py -m usb_replug ports
py -m usb_replug status
py -m usb_replug off
py -m usb_replug on
py -m usb_replug cycle --delay-ms 2000
py -m usb_replug --port COM12 status
py -m usb_replug --port COM12 off
py -m usb_replug --port COM12 on
py -m usb_replug --port COM12 cycle --delay-ms 2000
py -m usb_replug --port COM12 fault
py -m usb_replug --port COM12 version
```

When `--port` is omitted, the CLI auto-discovers a pyserial port matching USB
VID/PID `0x1a86:0x5722`. Override the IDs with decimal or `0x` hex values, and
use `--serial` to require a serial-number substring:

```powershell
py -m usb_replug --vid 0x1A86 --pid 0x5722 ports
py -m usb_replug --serial ABC123 status
```

Auto-discovery fails clearly if no port or more than one port matches. Passing
`--port` always takes precedence over auto-discovery.

The CLI requires `pyserial` when talking to real hardware:

```powershell
py -m pip install pyserial
```

The firmware command protocol uses plain ASCII lines and returns one response line per command.

The CLI rejects `cycle --delay-ms` values outside the firmware
`0.2.1-revc1` range of 100 to 30000 ms before opening the serial port.
For every `cycle`, the serial timeout is raised automatically to at least the
effective delay plus 2 seconds; a cycle without `--delay-ms` uses the firmware
default of 2000 ms. Device `ERR ...` responses are written to stderr and return
exit status 1.
