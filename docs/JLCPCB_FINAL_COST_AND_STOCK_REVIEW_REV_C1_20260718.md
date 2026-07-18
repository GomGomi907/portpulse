# USB Replug Dongle Rev.C.1 — Final JLCPCB Cost/Stock Review

Date: 2026-07-18  
Order target: 5 PCBs / 5 fully assembled top-side PCBAs  
Board: 4-layer, 49.5 mm × 21.6 mm

## Final design decisions

- Use CH334U `C5187524` in QSOP-28 instead of CH334R `C4154405`.
  The logged-in JLCPCB review showed only 2 assembly-stock units for CH334R,
  which cannot satisfy a five-PCBA order. CH334U showed 1,101 units and avoids
  QFN X-ray cost.
- Replace the unavailable 10 kΩ `C25804` group with the already-used Basic
  5.1 kΩ `C23186`. R1/R2/R4-R9 now share one line item. The equal R6/R8/R9
  ratio preserves the reset-safe FSUSB42 `/OE` limits.
- Keep USB-C upstream `C7507405` and USB-A female downstream `C8328`.
  The latter showed 13,558 units during the logged-in review. Both exact
  footprints and mating directions are verified in the connector audit.
- Keep all five PCBAs assembled. Setup, stencil, and regular-Extended feeder
  charges are order-level costs; leaving one board bare does not materially
  reduce them and removes one usable/debug unit.
- Keep four layers. The PCB remains under the 50 mm promotion boundary while
  preserving an uninterrupted In1.Cu GND reference under USB 2.0 HS paths.

## BOM cost structure

Regular Extended types are limited to seven:

1. J1 USB-C connector
2. J2 USB-A connector
3. U1 CH334U hub
4. U2 CH552T MCU
5. U3 FSUSB42MUX
6. U4 TPS2552 power switch
7. U6/U7 USBLC6-2SC6 ESD array

Basic parts cover the LDO, crystal, all capacitors, and all resistors. The
final BOM has 14 rows and 29 fitted references; BOM and CPL reference sets are
identical and all placements are top side.

## Quote options

- Economic PCBA, top side only
- Quantity 5 PCB / 5 PCBA
- Disable optional nitrogen reflow unless JLCPCB requires it
- Keep engineer placement review enabled
- Do not select X-ray or double-sided assembly unless the UI requires it
- Confirm connector mixed SMT/PTH shell-pad processing in the engineering
  preview

## Live order gate

Stock and pricing are time-sensitive. Immediately before cart save/payment:

1. Re-check every LCSC row and reject any automatic substitution that changes
   electrical rating, pinout, package, or connector geometry.
2. Confirm U1 is `C5187524`, the 5.1 kΩ group is `C23186`, J1 is `C7507405`,
   and J2 is `C8328`.
3. Confirm no obsolete `C25744`, unavailable `C25804`, or short-stock
   `C4154405` is selected.
4. Verify exactly one linked PCB/PCBA pair exists in the cart and all checkout
   checkboxes remain clear.
5. Record the final PCB, assembly, components, Extended feeder, special
   component, review, optional-process, shipping, and total amounts.

Payment/checkout is outside this release gate and requires the user.
