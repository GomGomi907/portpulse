# PortPulse native host

Small native C host for the PortPulse CH552 CDC interface. It uses the OS
serial driver already provided by Windows or Linux; no Python runtime is
required.

Build with CMake:

```sh
cmake -S . -B build
cmake --build build --config Release
```

Examples:

```text
portpulse --port COM12 status
portpulse --port COM12 on
portpulse --port COM12 off
portpulse --port COM12 --delay-ms 2000 cycle
```

The current Rev.C.1 firmware exposes one logical target port. Device
auto-discovery and `--device <serial>` selection are planned for the next
host iteration; explicit port selection is intentional until multi-device
enumeration is added.
