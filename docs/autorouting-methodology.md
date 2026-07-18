# Professional Autorouting Methodology — USB Replug Dongle Rev.A

## Purpose

This document defines how this KiCad project should evolve from connectivity-only autorouting to evidence-based, manufacturable, reviewable routing. It is the source-of-truth methodology for later scripts and routing-intent files.

Baseline project evidence:
- Board: `USB_Replug_Dongle_RevA/USB_Replug_Dongle_RevA.kicad_pcb`
- Baseline metrics: `.omx/reports/autorouting-ultragoal-g001-baseline.json`
- Prior research: `.omx/reports/deep-research-autorouting-rules-20260622.md`

## Evidence Hierarchy

1. **Hard manufacturing limits**: selected JLCPCB stack-up/copper/capability limits. JLCPCB tells users to set KiCad rules for minimum track width, spacing, via sizes, drill diameters, and other critical parameters before manufacturing export and to rerun DRC. Source: <https://jlcpcb.com/help/article/how-to-generate-gerber-and-drill-files-in-kicad-9>
2. **High-speed electrical rules**: USB 2.0 DP/DM route impedance, symmetry, continuous plane, via/corner minimization. Microchip states USB 2.0 DP/DM should maintain nominal 90 ohm differential impedance and that width/spacing must be recalculated for dielectric thickness/copper/stack-up. Source: <https://ww1.microchip.com/downloads/en/AppNotes/AN13.10-Application-Note-DS00002972A.pdf>
3. **Component-specific layout rules**: TI TS3USB30E requires equal-length D+/D-, impedance match, minimum vias/corners, careful via clearance, avoidance of through-hole test points on twisted pair lines, avoidance near clocks/crystals/regulators, and continuous planes. Source: <https://www.ti.com/lit/gpn/TS3USB30E>
4. **KiCad executable constraints**: net classes, custom rules, diff-pair gap/skew/length/uncoupled constraints, and DRC are the final machine-verifiable contract. Source: <https://docs.kicad.org/9.0/en/pcbnew/pcbnew.html>
5. **External autorouter limits**: Freerouting consumes DSN/SES and has net class/via/clearance/length concepts, but it does not inherently know datasheet intent unless encoded in rules, placement, fixed tracks, or postprocessing. Sources: <https://github.com/freerouting/freerouting>, <https://freerouting.org/freerouting/manual/routing-options>

## Routing Intent Model

Every net belongs to exactly one semantic class. A class compiles to KiCad net class/custom rules, Freerouting rules where possible, and local scoring rules.

| Class | Examples | Hard constraints | Soft cost priorities |
|---|---|---|---|
| `usb2_hs_pair` | `/USB_UP_DP_RAW` + `/USB_UP_DM_RAW`, hub-switch-DUT pairs | JLC-derived width/gap for 90 ohm differential, continuous reference plane, no clock/crystal/regulator keepout violation, DRC clean | fewest vias, shortest route, pair symmetry, low skew, low uncoupled length, native arc or two-45 corners |
| `usb_fs_internal` | MCU USB downstream control path | width/clearance, DRC clean, ESD/connector proximity if exposed | short path, few bends, avoid noisy areas |
| `vbus_power` | `/VBUS_IN`, `/VBUS_DUT_SW`, `/VBUS_DUT` | min width from current/temperature policy, no neckdown below policy, switch topology order | short/wide trunk, minimal via bottlenecks, large clearance margin |
| `logic_power` | `/3V3` | DRC clean, decoupling connectivity | short cap fanout, low via count, plane use |
| `crystal_clock` | `/HUB_XTALIN`, `/HUB_XTALOUT` | short local routing, keep USB away from crystal region | compact symmetry, low loop area |
| `control_gpio` | EN/OE/FAULT/LED/reset/strap | DRC clean, reset/strap deterministic pull state | route after critical nets, low visual clutter |
| `ground` | `/GND` | continuous reference plane and GND stitching where needed | preserve unbroken plane under USB |

## Router Pipeline

1. **Preflight**
   - Load board and schematic artifacts.
   - Confirm selected JLC stack-up and KiCad board constraints.
   - Confirm existing baseline DRC/ERC evidence or rerun when generating release changes.

2. **Placement locks**
   - Lock connectors, hub, USB data switch, ESD, power switch, crystal, and debug header before routing experiments.
   - Keep ESD close to exposed connector paths and keep crystal local.

3. **Critical-net routing first**
   - Route USB pairs before low-speed nets.
   - Route VBUS/power trunks before GPIO.
   - Route crystal/clock locally and protect it with USB keepout.

4. **External completion**
   - Export DSN only after critical nets are fixed or scored.
   - Use Freerouting for low-risk remaining nets.
   - Import SES and immediately run route audit + DRC.

5. **Post-route beautification**
   - Apply native arcs/fillets only after routing is stable.
   - Keep an editable source board and generate rounded output/copy.
   - KiCad warns that tracks with arcs are immovable by the shove router, so arc conversion is a finalization step rather than an early routing primitive. Source: <https://docs.kicad.org/9.0/en/pcbnew/pcbnew.html>

6. **Verification gates**
   - KiCad DRC: 0 violations, 0 unconnected pads, 0 footprint errors unless explicitly waived.
   - Route audit: score does not regress for critical nets; USB pair skew/uncoupled/via count within configured limits.
   - Manufacturing audit: JLC constraints remain compatible with selected process.

## Scoring Model

The scoring model intentionally separates **hard fail** from **soft quality**.

### Hard Fail

A route fails if any of these are true:
- KiCad DRC reports unwaived violations.
- A critical net is unrouted or disconnected.
- A USB pair violates configured diff-pair width/gap/skew/uncoupled/via-count constraints.
- A USB route crosses a configured keepout near crystal/clock/switching-regulator regions.
- A power route has a segment narrower than configured current policy.

### Soft Score

Lower score is better:

```text
score =
  1.0 * total_length_mm
+ 8.0 * via_count
+ 5.0 * layer_change_count
+ 1.5 * corner_count
+ 4.0 * non_45_or_arc_corner_count
+ 0.5 * small_segment_count
+ 10.0 * usb_pair_skew_mm
+ 6.0 * usb_pair_uncoupled_mm
- 2.0 * clearance_margin_bonus
- 3.0 * preferred_layer_bonus
- 2.0 * native_arc_corner_bonus
```

Weights are starting values for this project. They should be calibrated by comparing baseline board metrics and DRC-clean alternatives.

## Arc/Rounded Corner Policy

| Net class | Policy |
|---|---|
| `usb2_hs_pair` | Prefer native arcs after routing; fallback to two 45-degree turns. TI explicitly allows two 45-degree turns or an arc instead of a single 90-degree turn for USB high-speed signals. Source: <https://www.ti.com/lit/gpn/TS3USB30E> |
| `vbus_power` | Use chamfer/arc only if it does not reduce width or create clearance/thermal bottleneck. |
| `control_gpio` | Use 45-degree or arc by aesthetic score; never sacrifice critical-net clearance. |

## Deterministic Artifacts

The methodology compiles into:

- `routing/routing_sources.yaml` — cited source rules and extraction notes.
- `routing/routing_intent.yaml` — net classes, pair definitions, width/gap/via/skew/corner policies.
- `tools/route_quality_audit.py` — metric and scoring tool.
- `.omx/reports/autorouting-route-quality-current.json` — current board metrics.
- `.omx/reports/autorouting-route-quality-current.md` — human review report.

## Non-goals for This Ultragoal

- Do not place JLCPCB orders or claim live order validation.
- Do not mutate the verified production board without a copy and fresh DRC evidence.
- Do not claim actual 90 ohm performance until a selected JLC stack-up is used to calculate and review width/gap.
