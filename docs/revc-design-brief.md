# USB Replug Dongle Rev.C — USB-stick-class redesign brief

Date: 2026-06-23

## Decision

Skip Rev.B and target Rev.C directly as a compact USB-stick-class product spin.

## Rev.C target

- Functional scope: keep Rev.A core function unchanged: PC sees a USB 2.0 hub, always-connected control MCU, and switched target USB port.
- Mechanical target: 55–62 mm length × 18–22 mm width.
- First layout envelope: 62 mm × 22 mm if retaining the current USB-A female footprint; shrink toward 55 mm × 20 mm after connector/MCU package selection.
- PCB: 4-layer FR-4, continuous inner GND reference for USB 2.0 HS.
- Assembly: JLCPCB full PCBA, top-side-first; bottom-side passives/debug pads allowed only if top-side placement cannot meet the envelope.
- Routing priority: USB HS, crystal, VBUS trunk/switch, 3V3 decoupling, then low-speed GPIO/LED/debug.

## Connector architecture

Primary Rev.C mechanical architecture:

- Upstream: USB-C plug or compact USB-C upstream connector variant, USB 2.0-only UFP/Sink behavior.
- Downstream: USB-A female for DUT compatibility.

Mechanical fallback if JLC sourcing/assembly of a USB-C plug is poor:

- Keep a compact USB-C receptacle for Rev.C-P0 fit validation.
- Preserve the same electronics and layout zones so the upstream connector can be swapped after part availability is confirmed.

Rationale:

- Current Rev.A pad survey shows the USB-A female is the dominant size limiter.
- Current J2 pad box is about 14.0 mm × 9.42 mm; including body/courtyard it drives a realistic 18–22 mm board width.
- USB-stick-class is therefore realistic without changing the downstream connector, but aggressive 17–18 mm widths require connector-specific keepout checking.

## Component/package strategy

| Block | Rev.A state | Rev.C direction |
|---|---|---|
| USB hub | USB2422 QFN 4×4 | Keep if JLC availability/assembly is acceptable; already compact |
| MCU | STM32F042 TSSOP-20 | Prefer QFN/UFQFPN STM32F042 variant or equivalent USB-FS MCU; keep firmware model |
| Data switch | TS3USB30E VSSOP-10 | Keep or migrate to same-function smaller package only if JLC-safe |
| VBUS switch | TPS2553 SOT-23-6 | Keep unless part availability/cost drives replacement |
| LDO | AP2112K SOT-23-5 | Keep or smaller compatible LDO if stock favors it |
| ESD | SOT-23-6 USB ESD | Prefer lower-profile low-capacitance arrays near connectors |
| Passives | 0603 | Move to 0402 for Rev.C density, except current-limit/USB tuning pads where hand-rework is useful |
| Debug | Tag-Connect + buttons | Replace with tiny pogo/SWD pads; remove large reset button; boot via pad/jumper |
| LEDs | 3 LEDs | Reduce to 1–2 LEDs or edge-visible 0402 LEDs |

## Placement zoning

Left-to-right signal flow for the compact layout:

1. Upstream USB connector / plug
2. Upstream ESD and CC/Rd network
3. USB hub + 24 MHz crystal adjacent
4. MCU in parallel branch off hub port 1
5. Data switch and downstream ESD near downstream connector
6. VBUS switch near downstream connector power pin
7. USB-A downstream at board end

Keep USB HS traces short, symmetric, and over unbroken GND. Do not let the auto-router route USB, crystal, or switched VBUS before those classes are manually/fixed routed.

## JLCPCB constraints to preserve

- 0402 is within JLCPCB assembly capability, but use order-time preview and part selection.
- BOM and CPL must keep exact refdes matching.
- Through-hole/mechanical connector pads must be explicitly accepted by JLC engineering preview if used.
- Current Rev.A 0.30/0.15 mm vias are manufacturable-edge; Rev.C should prefer larger vias where possible, e.g. 0.45/0.20 mm, unless density forces smaller.
- Controlled-impedance dimensions must be selected from the actual JLC stack-up/impedance calculator before release.

Sources checked during planning:

- JLCPCB PCB Assembly Capabilities: https://jlcpcb.com/capabilities/pcb-assembly-capabilities
- JLCPCB PCB Capabilities: https://jlcpcb.com/capabilities/pcb-capabilities
- JLCPCB PCB Assembly FAQs: https://jlcpcb.com/help/article/pcb-assembly-faqs
- JLCPCB PCB Assembly FAQs Part 2: https://jlcpcb.com/help/article/pcb-assembly-faqs-part-2
- JLCPCB via design best practices: https://jlcpcb.com/blog/pcb-via-design-best-practices

## Phase gates

### Rev.C Phase 0 — package and mechanical lock

Exit gate:

- Target connector footprints chosen.
- Board outline target fixed.
- All assembled parts have JLC/LCSC candidates or explicit order-time sourcing notes.
- USB stick envelope drawing/placement keepouts saved.

### Rev.C Phase 1 — schematic/BOM

Exit gate:

- ERC 0 or documented waivers.
- Package substitutions reflected in symbol pin mapping and footprints.
- JLC BOM metadata complete for assembled parts.

### Rev.C Phase 2 — compact PCB

Exit gate:

- DRC 0, unconnected 0, schematic parity 0.
- Route quality audit passes hard checks.
- Soft warnings are documented and tied to Rev.C risk log.

### Rev.C Phase 3 — manufacturing release

Exit gate:

- Fresh ERC/DRC immediately before export.
- Gerber/drill/BOM/CPL package audit PASS.
- Release notes explicitly call out connector through-hole/mechanical soldering, polarity, rotations, stock, and impedance settings.

## Immediate next implementation step

Create `USB_Replug_Dongle_RevC` as a separate KiCad project so Rev.A manufacturing files remain frozen. Start with a compact placement feasibility board, then replace high-impact packages/footprints before routing.

## Rev.C P1 — 55×20mm USB-stick feasibility update

Date: 2026-06-23

P1 now proves a 55.0 mm × 20.0 mm USB-stick-class placement envelope using the Rev.A electrical architecture with Rev.C package reductions:

- 0402-class R/C/LED footprints.
- compact SWD pogo pads instead of Tag-Connect.
- compact reset/boot solder/tweezer pads instead of a tact reset switch.
- 0.7 mm compact test pads.

Verification:

- Placement audit: `.omx/reports/revc-usb-stick-p1-placement-20260623.json`
  - pads outside outline: 0
  - body outside outline, excluding the upstream connector placeholder policy: 0
  - bounding-box overlaps: 0
  - unplaced/fallback refs: 0
- ERC: `.omx/reports/revc-p1-erc-20260623.rpt`, 0 errors / 0 warnings
- Visual export: `.omx/reports/revc-usb-stick-p1-placement-20260623.pdf`

P1 is not order-ready: routing is intentionally removed, and the 0402 passive LCSC BOM must be re-locked because Rev.A C-numbers such as `C19702` and `C14663` are 0603 examples.

Rev.C P1 BOM risk audit added: .omx/reports/revc-p1-bom-package-risk-20260623.md flags 54 inherited Rev.A passive/debug package-code rows that must be re-locked for 0402 before manufacturing release.

