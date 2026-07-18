# USB Replug Dongle Rev.C.1 Final Order-Ready Verification

Date: 2026-07-18 (Asia/Seoul)

## Decision

- Local engineering release: **PASS**
- JLCPCB live stock / quote / cart: **PASS**
- Payment / checkout: **NOT AUTHORIZED**
- JLCPCB order-ready state: **PASS**, left in cart before checkout.

## Hardware verification

- KiCad: 9.0.6
- Board: 49.5 mm x 21.6 mm, 4 layers
- ERC: 0 errors, 0 warnings
- DRC: 0 violations, 0 unconnected pads, 0 footprint errors
- Route audit: PASS, 0 warnings
- Differential-pair gates: all PASS
  - USB_UP_RAW: 1.200 mm skew / 1.500 mm limit, 0/0 vias
  - USB_UP_HUB: 0.750 mm / 1.000 mm, 0/0 vias
  - MCU_USB_FS: 0.661 mm / 2.000 mm, 2/2 vias
  - HUB_DUT: 0.056 mm / 1.000 mm, 0/0 vias
  - DUT_SWITCH: 0.862 mm / 1.000 mm, 0/0 vias
  - DUT_CONNECTOR: 0.042 mm / 1.000 mm, 0/0 vias
- In1.Cu remains the GND reference plane; the long upstream VBUS trunk is on In2.Cu.
- Reset-domain audit: PASS
  - CH552 P3.1 is unconnected.
  - CH334U reset is only U1.17 + TP8.
  - TARGET_FAULT_N is in the 5 V domain and is not tied to 3V3.
  - FSUSB42 /OE divider calculated POR maximum: 2.6513 V, below 3.3 V absolute maximum.
  - Released minimum: 1.5623 V, above 1.3 V VIH minimum.
- Connector orientation audit: PASS
  - J1 USB-C upstream opens left toward the PC.
  - J2 USB-A female downstream opens right toward the DUT.
- High-resolution visual evidence:
  - `.omx/reports/revc1/visuals/top-assembly-inspect-hi.png`
  - `.omx/reports/revc1/visuals/bottom-copper-inspect-hi.png`

## JLCPCB package verification

- Clean-generated release:
  - `production/USB_Replug_Dongle_RevC1_JLCPCB_OrderReady_20260718`
- PCB upload:
  - `production/USB_Replug_Dongle_RevC1_JLCPCB_OrderReady_20260718/USB_Replug_Dongle_RevC1_JLCPCB_OrderReady_20260718_gerber_drill.zip`
- Assembly BOM:
  - `production/USB_Replug_Dongle_RevC1_JLCPCB_OrderReady_20260718/assembly/bom.csv`
- Assembly CPL/POS:
  - `production/USB_Replug_Dongle_RevC1_JLCPCB_OrderReady_20260718/assembly/positions.csv`
- Full release:
  - `production/USB_Replug_Dongle_RevC1_JLCPCB_OrderReady_20260718_FULL_RELEASE.zip`
- Package audit: PASS
  - 16 Gerber/drill files
  - 14 BOM rows
  - 29 BOM references
  - 29 CPL references
  - BOM/CPL reference sets exactly match
  - No blank LCSC IDs
  - Top-side placement only
  - Assembly note: 220 / 500 characters
  - No active `C25744`, `C25804`, `C4154405`, or `CH334R`
- ZIP and manifest audit: PASS
  - Gerber ZIP SHA256: `6CAC9D513B0341766E7C2989DBD89D8055B379E69DD07F3453C60F4855EF2A31`
  - Assembly ZIP SHA256: `FDB6252F8063E1923F6F2B85874C8F09CB350C59847E999AF33D1EB8ECEFE83E`
  - Full release ZIP SHA256: `EB85C8252D5551C67B9F9A2CA2FB962723F9D101DD1DD118982638C981A836F7`
  - No duplicate entries, empty entries, read/CRC errors, missing manifest files, or hash mismatches

## Active assembly parts

- U1: CH334U, `C5187524`
- U2: CH552T, `C111367`
- U3: FSUSB42MUX, `C11355`
- U4: TPS2552DBVR, `C46506`
- U5: XC6206P332MR-G, `C5446`
- U6/U7: USBLC6-2SC6, `C7519`
- J1: USB-C, `C7507405`
- J2: USB-A female, `C8328`
- R1/R2/R4-R9: 5.1 kOhm 1%, `C23186`
- R10: 51 kOhm 1%, `C107231`

The old out-of-stock `C25744` 10 kOhm row is not used. The reset-safe resistor network and USB-C CC resistors are consolidated into stocked 5.1 kOhm `C23186`.

## Firmware and host verification

- Firmware version: `0.2.1-revc1`
- Two clean SDCC builds: byte-for-byte reproducible
- Host C tests: 3 PASS
- GPIO safety tests: 2 PASS
- Python CLI tests: 16 PASS
- Code: 5897 / 14336 bytes
- Application XRAM: 79 / 768 bytes
- Highest image address: 0x1708, below 0x3800
- Firmware ZIP SHA256: `37225ECC2BB1FFBF4918625DBFD60FC98C07447F9D33B51DEC0B4F111A4F7611`
- Firmware release audit: PASS, 34 files, no CRC/duplicate/empty/hash error

## Mandatory live JLCPCB checks

1. Upload only the clean Rev.C.1 Gerber/BOM/CPL listed above.
2. Set 5 PCBs and 5 fully assembled PCBAs.
3. Use 4-layer Economic PCBA, top-side assembly.
4. Confirm every LCSC row is matched and in stock; the interface must contain no `C25744` error.
5. Confirm J1 opens left and J2 opens right in the placement preview.
6. Confirm U1/U2/U3/U4/U5/U6/U7 pin-1/orientation and Y1 orientation.
7. Remove optional nitrogen unless JLCPCB requires it; retain engineering placement review.
8. Leave exactly one current PCB + PCBA linked pair in the cart.
9. Leave every cart checkbox unselected.
10. Do not proceed to checkout or payment.

All ten live checks passed on 2026-07-18. Detailed evidence:

- `.omx/reports/revc1/revc1-jlcpcb-live-cart-final.md`
- `.omx/reports/revc1/browser-evidence/03-placement-auto-aligned.png`
- `.omx/reports/revc1/browser-evidence/04-placement-2d-top.png`
- `.omx/reports/revc1/browser-evidence/05-final-cart-unchecked.png`

Live order state:

- Exact linked PCB/PCBA suffix: `9897851A`
- PCB quantity: 5
- Fully assembled Economic top-side PCBA quantity: 5
- Cart groups: 1
- Visible cart checkboxes: 3, all unchecked
- PCB subtotal: $2.00
- PCBA subtotal: $53.11
- Total before shipping: $55.11
- Shipping: not calculated because the cart remains unchecked
- Checkout/payment: not performed
- `C25744`, `C25804`, `C4154405`, and `CH334R`: absent
- J2 `C8328`: exact part selected manually; live JLCPCB stock 13,558, idle stock 409

## Remaining post-receipt gate

No software-only review can replace physical bring-up. After assembly, verify rails, current limit/fault behavior, hub + MCU USB enumeration, DUT disconnect/reconnect behavior, and the 100-cycle test matrix before treating the design as field-qualified.
