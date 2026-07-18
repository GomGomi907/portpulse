"""Regression checks for reset-safe CH552 GPIO mode transitions."""

from pathlib import Path
import unittest


SOURCE = Path(__file__).resolve().parents[1] / "src" / "ch552_main.c"


class GpioSafetyTests(unittest.TestCase):
    def test_p3_fault_and_unused_pin_become_high_impedance(self) -> None:
        source = SOURCE.read_text(encoding="utf-8")
        statements = (
            "P3 |= TARGET_FAULT_MASK | UNUSED_P3_1_MASK;",
            "P3_DIR_PU &= (uint8_t)~(TARGET_FAULT_MASK | UNUSED_P3_1_MASK);",
            "P3_MOD_OC &= (uint8_t)~(TARGET_FAULT_MASK | UNUSED_P3_1_MASK);",
        )
        for statement in statements:
            self.assertIn(statement, source)

        latch_high, direction_clear, input_mode = (
            source.index(statement) for statement in statements
        )
        self.assertLess(latch_high, direction_clear)
        self.assertLess(direction_clear, input_mode)
        self.assertNotIn("HUB_RESET_MASK", source)
        self.assertNotIn("board_hub_reset_set", source)

    def test_p1_power_is_push_pull_but_data_control_is_open_drain(self) -> None:
        source = SOURCE.read_text(encoding="utf-8")
        latch_high = source.index("P1 |= TARGET_PWR_MASK | TARGET_DATA_MASK;")
        data_no_pull = source.index(
            "P1_DIR_PU &= (uint8_t)~TARGET_DATA_MASK;"
        )
        data_open_drain = source.index("P1_MOD_OC |= TARGET_DATA_MASK;")
        power_push_pull = source.index(
            "P1_MOD_OC &= (uint8_t)~TARGET_PWR_MASK;"
        )
        power_output = source.index("P1_DIR_PU |= TARGET_PWR_MASK;")
        self.assertLess(latch_high, data_no_pull)
        self.assertLess(data_no_pull, data_open_drain)
        self.assertLess(latch_high, power_push_pull)
        self.assertLess(power_push_pull, power_output)


if __name__ == "__main__":
    unittest.main(verbosity=2)
