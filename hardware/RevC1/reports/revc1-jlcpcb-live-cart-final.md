# Rev.C.1 JLCPCB Live Cart Final Verification

- Date: 2026-07-18 (Asia/Seoul)
- Verdict: **PASS**
- Scope: 5 PCB + 5 fully assembled PCBA, 4-layer Economic, top-side assembly
- Safety boundary: checkout/payment was not entered or performed

## Uploaded Rev.C.1 production files

| File | SHA-256 |
| --- | --- |
| `USB_Replug_Dongle_RevC1_JLCPCB_OrderReady_20260718_gerber_drill.zip` | `6CAC9D513B0341766E7C2989DBD89D8055B379E69DD07F3453C60F4855EF2A31` |
| `assembly/bom.csv` | `A1331FF0323D9E58D634FFF232963443D5E3F201F3BE2198E1995F59E3411CD3` |
| `assembly/positions.csv` | `DFCEBD58D2873A6FD562ABC5523707EB8AB6C0F7B3BF3549781511A473CA3BE5` |

The live Gerber parser detected a 4-layer board measuring 21.6 × 49.5 mm, equivalent to the source 49.5 × 21.6 mm orientation.

## Live configuration

- PCB quantity: 5
- PCBA quantity: 5
- PCB layers: 4
- Assembly service: Economic
- Assembly side: Top Side
- Fully assembled PCBA: enabled
- Engineer placement review: enabled
- Automatic placement confirmation: disabled
- Assembly terms: accepted
- Economic-mode nitrogen reflow: `Yes (for Economic)`; the UI disabled `No`
- PCBA remark: saved; its charge remains `Quote after review`
- Customs description: `Reserch\Education\DIY\Entertainment / Development Board - HS Code 847330`

## Live BOM match and availability

All 14 BOM rows were selected with the exact requested LCSC part number and a positive procurement quantity.

| References | Exact LCSC | Procurement quantity | Live result |
| --- | --- | ---: | --- |
| C1, C10 | C19702 | 10 | Selected |
| C2, C3 | C15849 | 10 | Selected |
| C4-C9 | C14663 | 30 | Selected |
| J1 | C7507405 | 5 | Selected |
| J2 | C8328 | 6 | Selected; JLCPCB stock 13,558 and idle stock 409 |
| R1, R2, R4-R9 | C23186 | 40 | Selected |
| R10 | C107231 | 20 | Selected |
| U1 | C5187524 | 5 | Selected |
| U2 | C111367 | 5 | Selected |
| U3 | C11355 | 5 | Selected |
| U4 | C46506 | 5 | Selected |
| U5 | C5446 | 5 | Selected |
| U6, U7 | C7519 | 12 | Selected |
| Y1 | C9002 | 5 | Selected |

J2 was explicitly selected as exact part `C8328`; no substitute was used. The current live order and final cart contain none of the stale/forbidden identifiers `C25744`, `C25804`, `C4154405`, or `CH334R`.

## Placement and rotation verification

- 29/29 placements are mounted, on layer `T`, and mapped to the exact intended LCSC ID.
- After JLCPCB auto-alignment, the global coordinate transform was consistent: X was preserved for 28/29 placements; J1 alone had a -0.125 mm X model-anchor offset. All 29 Y coordinates followed the JLCPCB sign-inverted coordinate frame.
- Key final rotations: J1 270°, J2 90°, U1 90°, U2 270°, U3 270°, U4 0°, U5 180°, U6 270°, U7 270°, Y1 0°.
- The corrected 2D and 3D previews place J1 opening left toward the PC and J2 opening right toward the DUT. IC/connector pin-1 and orientation markers align with their footprints.
- J1/J2 mixed SMT/PTH shell pads and models were accepted by the placement editor.

## Exact live quote breakdown

### PCB

| Line | Price |
| --- | ---: |
| PCB Price | $2.00 |
| Via Covering | $0.00 |
| Confirm Production file | $0.00 |
| PCB build time, 3 days | $0.00 |
| **PCB subtotal** | **$2.00** |

The final cart shows a struck reference PCB price of $7.00 and a Special offer price of $2.00, an exact displayed discount of **$5.00**.

### Economic PCBA

| Line | Price |
| --- | ---: |
| Setup Fee | $8.18 |
| Stencil | $1.53 |
| Panel | $0.00 |
| Large Size | $0.00 |
| Components, 14 items | $16.09 |
| Extended components fee | $24.56 |
| SMT Assembly | $1.22 |
| Confirm Parts Placement | $0.45 |
| Special components fee | $0.18 |
| Nitrogen reflow soldering | $0.90 |
| Assembly lead time, 3-4 days | $0.00 |
| **Economic PCBA subtotal** | **$53.11** |

The priced PCBA lines sum exactly to $53.11. The PCBA remark is shown as `Quote after review` and is not included in the total.

### Total and shipping

- **Order quote total before shipping: $55.11**
- Calculated weight: 291.90 g
- Shipping Estimated: `--`
- Final unchecked-cart Merchandise Total: `$0.00`
- Final unchecked-cart Subtotal: `$0.00`

Shipping is not quoted while every cart checkbox remains unselected. No checkbox was selected merely to obtain a shipping price, and checkout was not entered.

## Final cart integrity

- Cart count: `All (1)` / `JLCPCB (1)`
- Exactly one cart group contains one linked PCB+PCBA pair:
  - PCB: `PCB prototype:Y6-9897851A`, quantity 5, $2.00
  - PCBA: `Economic PCBA: SMT026071860674-9897851A`, top-side, quantity 5, $53.11
- Both entries share link suffix `9897851A`.
- All three visible cart checkboxes were programmatically verified `checked=false`.
- No Secure Checkout, checkout, payment, or order submission action was performed.

## Evidence

- `browser-evidence/03-placement-auto-aligned.png`
- `browser-evidence/04-placement-2d-top.png`
- `browser-evidence/05-final-cart-unchecked.png`

## Final verdict

**PASS** — The new Rev.C.1 Gerber/BOM/CPL set is the sole current linked PCB+PCBA cart pair; the stale C25744 condition is absent; all exact BOM parts have live positive availability; placement/orientation is aligned; quantities and requested manufacturing settings are correct; and the final cart is fully unchecked with no checkout or payment.
