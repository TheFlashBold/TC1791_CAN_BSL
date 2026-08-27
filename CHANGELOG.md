# Changelog

## 2026-08-27 — Compressed writes and Raspberry Pi 5 support

### Target bootloader

- Added CAN command `0x08` for LZ4-compressed PFlash writes.
- Transfers independent blocks of up to 4096 uncompressed bytes and programs
  them as 256-byte PFlash pages.
- Validates page alignment, PFlash bank boundaries, block metadata, CAN frame
  sequence numbers, decompressed size, and flash-controller error flags.
- Added explicit ready, block-ready, block-complete, final, and error responses
  so the host waits for each target operation.
- Added a small heap-free LZ4 decoder suitable for the TC1791 bootloader.
- Preserved the existing read, password, erase, and uncompressed-write commands.

### Host tooling

- Added the `compressed_write <address> <length> <file>` REPL command.
- Added host-side protocol construction, response parsing, error reporting,
  transfer pacing, and progress display.
- Added page and bank validation before any transfer starts.
- Added protocol and Python-to-target-LZ4 interoperability tests.

### Raspberry Pi 5

- Replaced the legacy `pigpio` path with `gpiozero` and the `lgpio`
  character-device backend.
- RESET now uses BCM GPIO 22 (physical pin 15).
- BOOT_CFG now uses BCM GPIO 27 (physical pin 13) and is released to high
  impedance after the reset into BSL mode.
- Added a `gpiotest` REPL command for slow pin verification with a meter.
- Added timeouts and response validation to the SBOOT login path.
- Added clean shutdown of GPIO and SocketCAN resources.
- The two timing-sensitive 3.2 kHz SBOOT PWM waveforms must be supplied
  externally; the Pi 5 script controls RESET and BOOT_CFG only.

### Reproducible TriCore build

- Added the minimal TC1791 register definitions needed by the project, checked
  against the original bootloader in Ghidra.
- Added a freestanding runtime so proprietary DaVe headers and a target C
  library are not required.
- Updated the makefiles to build with a current bare-metal TriCore GCC and to
  generate `Bootloader_2/iRAM/Bootloader_2.bin` directly.
- Rebuilt the root `bootloader.bin` with compressed-write support. Its SHA-256
  is `64a9e6c2d09ab53b431aa1b938983337415a0664732016dd71d740367e922d28`.
- Added a reusable Ghidra script for importing the raw TC1791 PSPR image at
  address `0xC0000000`.

### Verification performed

- The target ELF and raw BIN build successfully with GCC 13.4 for TC1791.
- Ghidra confirms a valid TriCore little-endian image at
  `0xC0000000–0xC0002ECB`, including the command handlers and LZ4 decoder.
- Ruff, protocol tests, malformed-input tests, and LZ4 interoperability tests
  pass.
- A full ECU hardware write has not yet been validated. The complete target
  range must be erased before `compressed_write` is used.
