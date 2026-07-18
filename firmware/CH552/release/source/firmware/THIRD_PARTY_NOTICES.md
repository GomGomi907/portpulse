# Third-party notices

The firmware source in this directory is an original implementation written
from the CH552 datasheet and the USB-IF CDC class specification. No source from
the reference repositories under `.omx/research/revc1/third_party_refs` is
copied or linked into this firmware.

Build tools are separate works:

- SDCC is distributed under the GPL and is used only as the compiler/toolchain.
- Python and the host C compiler are used by the local build/test script.

The provisional USB VID/PID is an engineering default only. A product release
must use identifiers owned or licensed by the manufacturer.
