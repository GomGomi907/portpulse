from __future__ import annotations

import contextlib
import io
import sys
import types
import unittest
from types import SimpleNamespace
from unittest import mock

import usb_replug


def port(device: str, vid: int, pid: int, serial_number: str | None = None, description: str = "USB Serial") -> SimpleNamespace:
    return SimpleNamespace(
        device=device,
        vid=vid,
        pid=pid,
        serial_number=serial_number,
        description=description,
    )


class UsbReplugCliTests(unittest.TestCase):
    def parse(self, argv: list[str]):
        return usb_replug.make_parser().parse_args(argv)

    def test_builds_basic_commands(self) -> None:
        args = self.parse(["--port", "COM12", "status"])
        self.assertEqual(usb_replug.build_device_command(args), "status")

        args = self.parse(["--port", "COM12", "cycle"])
        self.assertEqual(usb_replug.build_device_command(args), "cycle")

        args = self.parse(["--port", "COM12", "cycle", "--delay-ms", "2000"])
        self.assertEqual(usb_replug.build_device_command(args), "cycle 2000")

    def test_rejects_bad_transport_settings(self) -> None:
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                self.parse(["--baud", "0", "status"])

            with self.assertRaises(SystemExit):
                self.parse(["--timeout", "-1", "status"])

    def test_accepts_decimal_and_hex_usb_ids(self) -> None:
        args = self.parse(["--vid", "6790", "--pid", "0x5722", "ports"])
        self.assertEqual(args.vid, 0x1A86)
        self.assertEqual(args.pid, 0x5722)

        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                self.parse(["--vid", "0x10000", "ports"])

    def test_rejects_out_of_range_cycle_delay(self) -> None:
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                self.parse(["--port", "COM12", "cycle", "--delay-ms", "99"])

            with self.assertRaises(SystemExit):
                self.parse(["--port", "COM12", "cycle", "--delay-ms", "30001"])

    def test_resolves_single_matching_port(self) -> None:
        args = self.parse(["status"])
        ports = [
            port("COM1", 0x1234, 0x5678),
            port("COM12", usb_replug.DEFAULT_VID, usb_replug.DEFAULT_PID, "ABC123"),
        ]

        with mock.patch.object(usb_replug, "import_list_ports", return_value=SimpleNamespace(comports=lambda: ports)):
            self.assertEqual(usb_replug.resolve_port(args), "COM12")

    def test_serial_substring_filters_matches(self) -> None:
        args = self.parse(["--serial", "RIGHT", "status"])
        ports = [
            port("COM11", usb_replug.DEFAULT_VID, usb_replug.DEFAULT_PID, "WRONG"),
            port("COM12", usb_replug.DEFAULT_VID, usb_replug.DEFAULT_PID, "LEFT-RIGHT"),
        ]

        with mock.patch.object(usb_replug, "import_list_ports", return_value=SimpleNamespace(comports=lambda: ports)):
            self.assertEqual(usb_replug.resolve_port(args), "COM12")

    def test_explicit_port_takes_precedence_over_discovery(self) -> None:
        args = self.parse(["--port", "COM99", "status"])

        with mock.patch.object(usb_replug, "import_list_ports") as import_list_ports:
            self.assertEqual(usb_replug.resolve_port(args), "COM99")

        import_list_ports.assert_not_called()

    def test_resolve_port_fails_clearly_for_zero_or_multiple_matches(self) -> None:
        args = self.parse(["status"])

        with mock.patch.object(usb_replug, "import_list_ports", return_value=SimpleNamespace(comports=lambda: [])):
            with self.assertRaisesRegex(RuntimeError, "no serial ports matched"):
                usb_replug.resolve_port(args)

        ports = [
            port("COM11", usb_replug.DEFAULT_VID, usb_replug.DEFAULT_PID),
            port("COM12", usb_replug.DEFAULT_VID, usb_replug.DEFAULT_PID),
        ]
        with mock.patch.object(usb_replug, "import_list_ports", return_value=SimpleNamespace(comports=lambda: ports)):
            with self.assertRaisesRegex(RuntimeError, "multiple serial ports matched.*COM11, COM12"):
                usb_replug.resolve_port(args)

    def test_ports_command_lists_matching_ports(self) -> None:
        ports = [
            port("COM1", 0x1234, 0x5678, "NOPE"),
            port("COM12", usb_replug.DEFAULT_VID, usb_replug.DEFAULT_PID, "ABC123", "USB Replug"),
        ]

        stdout = io.StringIO()
        with mock.patch.object(usb_replug, "import_list_ports", return_value=SimpleNamespace(comports=lambda: ports)):
            with contextlib.redirect_stdout(stdout):
                self.assertEqual(usb_replug.main(["ports"]), 0)

        self.assertEqual(stdout.getvalue().strip(), "COM12\tvid=0x1a86\tpid=0x5722\tserial=ABC123\tUSB Replug")

    def test_ports_command_fails_when_no_ports_match(self) -> None:
        stderr = io.StringIO()
        with mock.patch.object(usb_replug, "import_list_ports", return_value=SimpleNamespace(comports=lambda: [])):
            with contextlib.redirect_stderr(stderr):
                self.assertEqual(usb_replug.main(["ports"]), 2)

        self.assertIn("no serial ports matched VID:PID 0x1a86:0x5722", stderr.getvalue())

    def test_cycle_timeout_is_at_least_delay_plus_two_seconds(self) -> None:
        args = self.parse(["--timeout", "1", "cycle", "--delay-ms", "2000"])
        self.assertEqual(usb_replug.effective_timeout_s(args), 4.0)

        args = self.parse(["--timeout", "10", "cycle", "--delay-ms", "2000"])
        self.assertEqual(usb_replug.effective_timeout_s(args), 10.0)

    def test_default_cycle_timeout_accounts_for_firmware_delay(self) -> None:
        args = self.parse(["--timeout", "1", "cycle"])
        self.assertEqual(usb_replug.effective_timeout_s(args), 4.0)

    def test_main_prints_response_from_transport(self) -> None:
        calls = []
        original_transact = usb_replug.transact

        def fake_transact(port: str, baud: int, timeout_s: float, command: str) -> str:
            calls.append((port, baud, timeout_s, command))
            return "STATE off fault=0 version=0.1.0"

        try:
            usb_replug.transact = fake_transact
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                self.assertEqual(usb_replug.main(["--port", "COM12", "status"]), 0)
        finally:
            usb_replug.transact = original_transact

        self.assertEqual(calls, [("COM12", usb_replug.DEFAULT_BAUD, usb_replug.DEFAULT_TIMEOUT_S, "status")])
        self.assertEqual(stdout.getvalue().strip(), "STATE off fault=0 version=0.1.0")

    def test_main_auto_discovers_port_and_uses_cycle_timeout(self) -> None:
        calls = []
        ports = [port("COM12", usb_replug.DEFAULT_VID, usb_replug.DEFAULT_PID)]

        def fake_transact(port_name: str, baud: int, timeout_s: float, command: str) -> str:
            calls.append((port_name, baud, timeout_s, command))
            return "OK cycle"

        stdout = io.StringIO()
        with mock.patch.object(usb_replug, "import_list_ports", return_value=SimpleNamespace(comports=lambda: ports)):
            with mock.patch.object(usb_replug, "transact", side_effect=fake_transact):
                with contextlib.redirect_stdout(stdout):
                    self.assertEqual(usb_replug.main(["--timeout", "1", "cycle", "--delay-ms", "2500"]), 0)

        self.assertEqual(calls, [("COM12", usb_replug.DEFAULT_BAUD, 4.5, "cycle 2500")])
        self.assertEqual(stdout.getvalue().strip(), "OK cycle")

    def test_main_returns_failure_for_device_error_response(self) -> None:
        stdout = io.StringIO()
        stderr = io.StringIO()

        with mock.patch.object(usb_replug, "transact", return_value="ERR fault_active"):
            with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                self.assertEqual(usb_replug.main(["--port", "COM12", "on"]), 1)

        self.assertEqual(stdout.getvalue(), "")
        self.assertEqual(stderr.getvalue().strip(), "ERR fault_active")

    def test_transact_writes_ascii_command_and_reads_response(self) -> None:
        serial_instances = []

        class FakeSerialPort:
            def __init__(self, **kwargs):
                self.kwargs = kwargs
                self.writes = []
                serial_instances.append(self)

            def __enter__(self):
                return self

            def __exit__(self, exc_type, exc, tb):
                return False

            def reset_input_buffer(self):
                self.reset = True

            def write(self, payload):
                self.writes.append(payload)

            def flush(self):
                self.flushed = True

            def readline(self):
                return b"OK status\n"

        fake_serial_module = types.SimpleNamespace(Serial=FakeSerialPort)

        with mock.patch.dict(sys.modules, {"serial": fake_serial_module}):
            response = usb_replug.transact("COM12", 9600, 7.0, "status")

        self.assertEqual(response, "OK status")
        self.assertEqual(serial_instances[0].kwargs, {"port": "COM12", "baudrate": 9600, "timeout": 7.0, "write_timeout": 7.0})
        self.assertEqual(serial_instances[0].writes, [b"status\n"])


if __name__ == "__main__":
    unittest.main()
