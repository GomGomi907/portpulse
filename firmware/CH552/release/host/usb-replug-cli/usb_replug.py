"""USB Replug Dongle Rev.C.1 host CLI.

Run from this directory with:
    python -m usb_replug --port COM12 status
"""

from __future__ import annotations

import argparse
import sys
from typing import Sequence


DEFAULT_BAUD = 115200
DEFAULT_TIMEOUT_S = 5.0
DEFAULT_VID = 0x1A86
DEFAULT_PID = 0x5722
DEFAULT_CYCLE_DELAY_MS = 2000
MIN_CYCLE_DELAY_MS = 100
MAX_CYCLE_DELAY_MS = 30000


def positive_int(value: str) -> int:
    try:
        parsed = int(value, 10)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("value must be an integer") from exc

    if parsed <= 0:
        raise argparse.ArgumentTypeError("value must be greater than zero")

    return parsed


def positive_float(value: str) -> float:
    try:
        parsed = float(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("value must be a number") from exc

    if parsed <= 0.0:
        raise argparse.ArgumentTypeError("value must be greater than zero")

    return parsed


def usb_id(value: str) -> int:
    try:
        parsed = int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("value must be decimal or 0x-prefixed hexadecimal") from exc

    if not 0 <= parsed <= 0xFFFF:
        raise argparse.ArgumentTypeError("value must be between 0 and 0xffff")

    return parsed


def cycle_delay_ms(value: str) -> int:
    try:
        delay_ms = int(value, 10)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("delay must be an integer in milliseconds") from exc

    if not MIN_CYCLE_DELAY_MS <= delay_ms <= MAX_CYCLE_DELAY_MS:
        raise argparse.ArgumentTypeError(
            f"delay must be between {MIN_CYCLE_DELAY_MS} and {MAX_CYCLE_DELAY_MS} ms"
        )

    return delay_ms


def import_list_ports():
    try:
        from serial.tools import list_ports
    except ImportError as exc:
        raise RuntimeError("pyserial is required: install with `py -m pip install pyserial`") from exc

    return list_ports


def port_matches(port_info, vid: int, pid: int, serial_substring: str | None) -> bool:
    if port_info.vid != vid or port_info.pid != pid:
        return False

    if serial_substring is None:
        return True

    serial_number = port_info.serial_number or ""
    return serial_substring in serial_number


def matching_ports(vid: int, pid: int, serial_substring: str | None):
    list_ports = import_list_ports()
    return [
        port_info
        for port_info in list_ports.comports()
        if port_matches(port_info, vid, pid, serial_substring)
    ]


def format_port(port_info) -> str:
    fields = [
        port_info.device,
        f"vid=0x{port_info.vid:04x}",
        f"pid=0x{port_info.pid:04x}",
    ]

    if port_info.serial_number:
        fields.append(f"serial={port_info.serial_number}")
    if port_info.description:
        fields.append(str(port_info.description))

    return "\t".join(fields)


def resolve_port(args: argparse.Namespace) -> str:
    if args.port:
        return args.port

    matches = matching_ports(args.vid, args.pid, args.serial)
    selector = f"VID:PID 0x{args.vid:04x}:0x{args.pid:04x}"
    if args.serial:
        selector += f" serial containing {args.serial!r}"

    if not matches:
        raise RuntimeError(f"no serial ports matched {selector}; pass --port to select a port explicitly")

    if len(matches) > 1:
        devices = ", ".join(port_info.device for port_info in matches)
        raise RuntimeError(f"multiple serial ports matched {selector}: {devices}; pass --port or --serial")

    return matches[0].device


def build_device_command(args: argparse.Namespace) -> str:
    if args.command == "cycle":
        if args.delay_ms is None:
            return "cycle"
        return f"cycle {args.delay_ms}"
    return args.command


def effective_timeout_s(args: argparse.Namespace) -> float:
    if args.command == "cycle":
        delay_ms = args.delay_ms if args.delay_ms is not None else DEFAULT_CYCLE_DELAY_MS
        return max(args.timeout, delay_ms / 1000.0 + 2.0)

    return args.timeout


def transact(port: str, baud: int, timeout_s: float, command: str) -> str:
    try:
        import serial
    except ImportError as exc:
        raise RuntimeError("pyserial is required: install with `py -m pip install pyserial`") from exc

    with serial.Serial(port=port, baudrate=baud, timeout=timeout_s, write_timeout=timeout_s) as serial_port:
        serial_port.reset_input_buffer()
        serial_port.write((command + "\n").encode("ascii"))
        serial_port.flush()
        line = serial_port.readline()

    if not line:
        raise TimeoutError(f"timed out waiting for response to `{command}`")

    return line.decode("ascii", errors="replace").strip()


def make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="usb_replug", description="Control a USB Replug Dongle Rev.C.1 CDC port.")
    parser.add_argument("--port", help="CDC serial port, for example COM12 or /dev/ttyACM0. Overrides auto-discovery.")
    parser.add_argument("--baud", type=positive_int, default=DEFAULT_BAUD, help=f"Serial baud rate, default {DEFAULT_BAUD}.")
    parser.add_argument(
        "--timeout",
        type=positive_float,
        default=DEFAULT_TIMEOUT_S,
        help=f"Read/write timeout in seconds, default {DEFAULT_TIMEOUT_S}.",
    )
    parser.add_argument(
        "--vid",
        type=usb_id,
        default=DEFAULT_VID,
        help=f"USB vendor ID for auto-discovery, default 0x{DEFAULT_VID:04x}.",
    )
    parser.add_argument(
        "--pid",
        type=usb_id,
        default=DEFAULT_PID,
        help=f"USB product ID for auto-discovery, default 0x{DEFAULT_PID:04x}.",
    )
    parser.add_argument("--serial", help="Optional serial-number substring for auto-discovery.")

    subparsers = parser.add_subparsers(dest="command", required=True)
    for name in ("status", "on", "off", "fault", "version", "help"):
        subparsers.add_parser(name)
    subparsers.add_parser("ports")

    cycle = subparsers.add_parser("cycle")
    cycle.add_argument("--delay-ms", type=cycle_delay_ms, default=None, help="Cycle off interval in milliseconds.")

    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = make_parser().parse_args(argv)

    try:
        if args.command == "ports":
            ports = matching_ports(args.vid, args.pid, args.serial)
            if not ports:
                raise RuntimeError(f"no serial ports matched VID:PID 0x{args.vid:04x}:0x{args.pid:04x}")
            for port_info in ports:
                print(format_port(port_info))
            return 0

        command = build_device_command(args)
        port = resolve_port(args)
        response = transact(port, args.baud, effective_timeout_s(args), command)
    except Exception as exc:
        print(f"usb_replug: {exc}", file=sys.stderr)
        return 2

    if response.startswith("ERR "):
        print(response, file=sys.stderr)
        return 1

    print(response)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
