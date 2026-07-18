# Rev.C.1 firmware offline verification

Date: 2026-07-18  
Verdict: **PASS**

## Evidence

- Portable C tests: 3/3 PASS
- GPIO safety tests: 2/2 PASS
- Python host CLI tests: 16/16 PASS
- Clean SDCC builds: 2/2 PASS and byte-for-byte reproducible
- Code: 5897 / 14336 bytes
- Application XRAM: 79 / 768 bytes
- USB DMA: EP0 `0x0000`, EP1 `0x0040`, EP2 `0x0080`
- Highest image address: `0x1708`, below bootloader boundary `0x3800`
- IHX/HEX checksums valid; BIN matches Intel HEX
- Reset-domain schematic audit: PASS

The exact logs and reproducibility hashes are included under `reports/`.

## Hardware acceptance still required

- Inspect connector/IC orientation and assembly quality
- Measure VBUS_IN, 3V3, TARGET_DATA_OE_N, TARGET_FAULT_N, and DUT VBUS
- Verify safe-off before and during MCU programming/reset
- Verify CH334U hub and CH552 CDC enumeration on Windows/Linux/macOS
- Exercise `on`, `off`, `cycle`, `status`, `fault`, and `version`
- Inject TPS2552 over-current and verify forced safe-off
- Complete 100 replug cycles for each representative DUT

No offline test can replace these physical checks.
