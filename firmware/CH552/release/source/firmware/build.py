"""Reproducible build and host-test driver for the Rev.C.1 CH552 firmware."""

from __future__ import annotations

import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "build"
SDCC_BUILD = BUILD / "sdcc"
HOST_BUILD = BUILD / "host"
SOURCES = (
    ROOT / "src" / "ch552_main.c",
    ROOT / "src" / "replug_core.c",
    ROOT / "src" / "usb_cdc.c",
    ROOT / "src" / "usb_descriptors.c",
)
CODE_LIMIT = 0x3800
XRAM_LIMIT = 0x0300


def run(command: list[str], *, env: dict[str, str] | None = None, capture: bool = False):
    printable = subprocess.list2cmdline(command)
    print(f"+ {printable}")
    return subprocess.run(
        command,
        cwd=ROOT,
        env=env,
        check=True,
        text=True,
        capture_output=capture,
    )


def find_executable(name: str, extra_candidates: tuple[Path, ...] = ()) -> Path:
    for candidate in extra_candidates:
        if candidate.is_file():
            return candidate
    discovered = shutil.which(name)
    if discovered:
        return Path(discovered)
    raise RuntimeError(f"required executable not found: {name}")


def sdcc_tools() -> tuple[Path, Path, Path, dict[str, str]]:
    configured = os.environ.get("SDCC_BIN")
    candidates: list[Path] = []
    if configured:
        candidates.append(Path(configured))
    candidates.extend(
        (
            Path(r"C:\Program Files\SDCC\bin"),
            Path(r"C:\Program Files (x86)\SDCC\bin"),
        )
    )

    sdcc = find_executable(
        "sdcc",
        tuple(candidate / "sdcc.exe" for candidate in candidates),
    )
    bin_dir = sdcc.parent
    packihx = find_executable(
        "packihx",
        (bin_dir / "packihx.exe",),
    )
    makebin = find_executable(
        "makebin",
        (bin_dir / "makebin.exe",),
    )

    env = os.environ.copy()
    path_entries = [str(bin_dir)]

    # SDCC 4.6.0's Windows package ships its private preprocessor backend as
    # extensionless "cc1". Some environments then resolve MinGW's unrelated
    # cc1.exe. Supply a workspace-local executable alias before the SDCC path.
    private_cc1 = bin_dir / "cc1"
    native_cc1 = bin_dir / "cc1.exe"
    if private_cc1.is_file() and not native_cc1.is_file():
        shim = BUILD / "tool-shim"
        shim.mkdir(parents=True, exist_ok=True)
        shim_cc1 = shim / "cc1.exe"
        shutil.copy2(private_cc1, shim_cc1)
        path_entries.insert(0, str(shim))

    path_entries.append(env.get("PATH", ""))
    env["PATH"] = os.pathsep.join(path_entries)
    return sdcc, packihx, makebin, env


def host_tests() -> None:
    gcc = find_executable("gcc")
    HOST_BUILD.mkdir(parents=True, exist_ok=True)
    common = [
        str(gcc),
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pedantic",
        "-Iinclude",
    ]
    core_test = HOST_BUILD / "test_replug_core.exe"
    descriptor_test = HOST_BUILD / "test_descriptors.exe"
    usb_cdc_test = HOST_BUILD / "test_usb_cdc.exe"
    run(common + ["src/replug_core.c", "tests/test_replug_core.c", "-o", str(core_test)])
    run([str(core_test)])
    run(common + ["src/usb_descriptors.c", "tests/test_descriptors.c", "-o", str(descriptor_test)])
    run([str(descriptor_test)])
    run(
        common
        + [
            "-DUSB_CDC_HOST_TEST",
            "src/usb_cdc.c",
            "src/usb_descriptors.c",
            "tests/test_usb_cdc.c",
            "-o",
            str(usb_cdc_test),
        ]
    )
    run([str(usb_cdc_test)])
    run([sys.executable, "tests/test_gpio_safety.py"])


def firmware_build() -> None:
    sdcc, packihx, makebin, env = sdcc_tools()
    SDCC_BUILD.mkdir(parents=True, exist_ok=True)
    flags = [
        "-mmcs51",
        "--model-small",
        "--std-c99",
        "--opt-code-size",
        "--xram-loc",
        "0x0100",
        "--xram-size",
        f"0x{XRAM_LIMIT:04x}",
        "--code-size",
        f"0x{CODE_LIMIT:04x}",
        "-Iinclude",
    ]

    objects: list[Path] = []
    for source in SOURCES:
        output = SDCC_BUILD / f"{source.stem}.rel"
        run(
            [str(sdcc), *flags, "-c", str(source.relative_to(ROOT)), "-o", str(output)],
            env=env,
        )
        objects.append(output)

    ihx = SDCC_BUILD / "usb_replug_ch552.ihx"
    run([str(sdcc), *flags, *(str(obj) for obj in objects), "-o", str(ihx)], env=env)

    packed = run([str(packihx), str(ihx)], env=env, capture=True)
    hex_file = SDCC_BUILD / "usb_replug_ch552.hex"
    hex_file.write_text(packed.stdout, encoding="ascii")
    binary = SDCC_BUILD / "usb_replug_ch552.bin"
    run([str(makebin), "-p", str(ihx), str(binary)], env=env)
    validate_memory_report(SDCC_BUILD / "usb_replug_ch552.mem")
    validate_dma_layout(SDCC_BUILD / "usb_replug_ch552.map")
    validate_firmware_artifacts(ihx, hex_file, binary)
    print(f"Firmware: {binary} ({binary.stat().st_size} bytes)")


def validate_memory_report(path: Path) -> None:
    text = path.read_text(encoding="utf-8", errors="replace")
    code_match = re.search(
        r"ROM/EPROM/FLASH\s+0x[0-9a-f]+\s+0x[0-9a-f]+\s+(\d+)\s+(\d+)",
        text,
        re.IGNORECASE,
    )
    xram_match = re.search(
        r"EXTERNAL RAM\s+0x[0-9a-f]+\s+0x[0-9a-f]+\s+(\d+)\s+(\d+)",
        text,
        re.IGNORECASE,
    )
    if not code_match or not xram_match:
        raise RuntimeError(f"unable to parse SDCC memory report: {path}")
    code_used, code_max = map(int, code_match.groups())
    xram_used, xram_max = map(int, xram_match.groups())
    if code_used > CODE_LIMIT or code_max != CODE_LIMIT:
        raise RuntimeError(f"code memory limit violated: {code_used}/{CODE_LIMIT}")
    if xram_used > XRAM_LIMIT or xram_max != XRAM_LIMIT:
        raise RuntimeError(f"application XRAM limit violated: {xram_used}/{XRAM_LIMIT}")
    print(f"Memory: code {code_used}/{code_max} bytes; application XRAM {xram_used}/{xram_max} bytes")


def validate_dma_layout(path: Path) -> None:
    text = path.read_text(encoding="utf-8", errors="replace")
    expected = {
        "_usb_ep0_buffer": 0x0000,
        "_usb_ep1_buffer": 0x0040,
        "_usb_ep2_buffer": 0x0080,
    }
    for symbol, address in expected.items():
        match = re.search(
            rf"^\s*([0-9a-f]{{8}})\s+{re.escape(symbol)}\s+",
            text,
            re.IGNORECASE | re.MULTILINE,
        )
        if not match:
            raise RuntimeError(f"DMA symbol missing from map: {symbol}")
        actual = int(match.group(1), 16)
        if actual != address:
            raise RuntimeError(
                f"DMA layout violation for {symbol}: 0x{actual:04x} != 0x{address:04x}"
            )
    print("DMA: EP0 0x0000, EP1 0x0040, EP2 0x0080")


def read_intel_hex(path: Path) -> dict[int, int]:
    memory: dict[int, int] = {}
    upper = 0
    eof_seen = False
    for line_number, raw_line in enumerate(path.read_text(encoding="ascii").splitlines(), 1):
        line = raw_line.strip()
        if not line.startswith(":"):
            raise RuntimeError(f"{path}:{line_number}: invalid Intel HEX record")
        record = bytes.fromhex(line[1:])
        if len(record) < 5 or sum(record) & 0xFF:
            raise RuntimeError(f"{path}:{line_number}: invalid Intel HEX checksum")
        length = record[0]
        if len(record) != length + 5:
            raise RuntimeError(f"{path}:{line_number}: invalid Intel HEX length")
        offset = (record[1] << 8) | record[2]
        record_type = record[3]
        data = record[4 : 4 + length]
        if record_type == 0x00:
            for index, value in enumerate(data):
                memory[upper + offset + index] = value
        elif record_type == 0x01:
            eof_seen = True
        elif record_type == 0x04:
            if length != 2:
                raise RuntimeError(f"{path}:{line_number}: invalid extended address")
            upper = ((data[0] << 8) | data[1]) << 16
    if not eof_seen:
        raise RuntimeError(f"{path}: missing Intel HEX EOF record")
    return memory


def validate_firmware_artifacts(ihx: Path, hex_file: Path, binary: Path) -> None:
    ihx_memory = read_intel_hex(ihx)
    hex_memory = read_intel_hex(hex_file)
    if ihx_memory != hex_memory:
        raise RuntimeError("packed HEX does not match linked IHX")
    if not ihx_memory:
        raise RuntimeError("firmware image is empty")
    max_address = max(ihx_memory)
    if max_address >= CODE_LIMIT:
        raise RuntimeError(
            f"firmware crosses bootloader boundary: 0x{max_address:04x} >= 0x{CODE_LIMIT:04x}"
        )
    expected = bytearray([0xFF] * (max_address + 1))
    for address, value in ihx_memory.items():
        expected[address] = value
    if binary.read_bytes() != expected:
        raise RuntimeError("binary image does not match Intel HEX contents")
    print(f"Image: checksums valid; highest address 0x{max_address:04x} < 0x{CODE_LIMIT:04x}")


def clean() -> None:
    if BUILD.exists():
        shutil.rmtree(BUILD)
    print(f"Removed {BUILD}")


def main(argv: list[str]) -> int:
    action = argv[1] if len(argv) > 1 else "all"
    if action == "clean":
        clean()
    elif action == "test":
        host_tests()
    elif action == "build":
        firmware_build()
    elif action == "all":
        host_tests()
        firmware_build()
    else:
        print("usage: py build.py [all|test|build|clean]", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
