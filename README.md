# PortPulse

PortPulse is a USB 2.0 replug dongle for automated hardware testing. It sits
between a host and a DUT and lets software disconnect and reconnect the DUT by
switching its VBUS and D+/D− while keeping the control MCU connected.

## Rev.C.1

- 4-layer, 49.5 mm × 21.6 mm PCB
- USB-C upstream, USB-A downstream
- USB 2.0 hub plus CH552 control MCU
- Independent target VBUS and USB data switching
- TPS2552 current limiting and fault reporting
- Top-side JLCPCB Economic PCBA release

The production release was checked with KiCad ERC/DRC, BOM/CPL reference-set
audits, route-quality checks, reset-domain analysis, ZIP/manifest integrity
checks, and firmware host/build tests. Physical bring-up and 100-cycle DUT
testing remain required after assembly.

## Repository layout

- `hardware/RevC1/kicad`: editable KiCad source
- `hardware/RevC1/jlcpcb-release`: JLCPCB Gerber, BOM, CPL, and release ZIPs
- `firmware/CH552/release`: firmware artifacts, source, tests, and bring-up notes
- `host/portpulse`: native C host utility for Windows/Linux CDC serial control
- `docs`: design and manufacturing notes

## License

Hardware, firmware, host software, and documentation are released under the
MIT License unless a file or bundled third-party notice states otherwise.
