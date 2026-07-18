# Rev.C.1 Reset-Domain Audit

- Verdict: PASS
- FSUSB /OE POR maximum: 2.6513 V (< 3.3 V)
- FSUSB /OE released minimum: 1.5623 V (> 1.3 V)
- CH334U RESET#/CDP: only U1.17 + TP8; CH552 P3.1 is NC.
- TPS2552 FAULT: U4.4 + U2.10 + TP7 with R7 pull-up to VBUS_IN.

## Checks

- [x] hub_reset_only_u1_and_tp8
- [x] ch552_p3_1_is_unconnected
- [x] fault_net_exact_members
- [x] fault_pullup_is_5v
- [x] fault_not_tied_to_3v3
- [x] oe_upper_pullup_is_5v
- [x] oe_series_path
- [x] oe_resistors_are_equal_5k1
- [x] oe_por_below_3v3
- [x] oe_release_above_vih_min
