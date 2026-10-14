# Changelog

## 2026-10-14 — Compressed DFlash/EEPROM writes and DFlash erase

### Target bootloader

- Extended CAN command `0x08` to write Data Flash (EEPROM) in addition to
  Program Flash. The target array and page size are selected from the
  destination address (PFlash: 256-byte pages; DFlash: 128-byte pages).
- Added `FLASH_writePageDFlash()`, which programs a 128-byte Data Flash page
  using the Data Flash `Enter Page Mode` command (`0x5D`) and 16 double-word
  loads. All command cycles are based at the addressed DFlash bank (A31–A16).
- Extended the erase command (`0x05`) to erase Data Flash sectors. DFlash
  addresses use `FLASH_erasePhysSector()` (the `Erase Phys Sector` command,
  confirm pattern `0x40`), because the plain `Erase Sector` command is rejected
  for DFlash. PFlash erase is unchanged. The two DFlash banks (DS0 at
  `0xAF000000`, DS1 at `0xAF080000`) are each a single `0x18000` sector erased
  by its base address.
- DFlash status is always read from `FLASH0_FSR`: both DFlash banks report there
  via D0BUSY (DS0) and D1BUSY (DS1), confirmed in the ASW. (An earlier draft
  wrongly used `FLASH1_FSR` for the `0xAF080000` bank, so its busy-wait would
  have returned immediately — a potential program/erase race.)
- Validated DFlash ranges against the two per-PMU banks (`0xAF000000` and
  `0xAF080000`, `0x18000` bytes each) with 128-byte alignment.
- Checked the DFlash operation/sequence/protection error bits (DFOPER, SQER,
  PROER) after each page program; PFlash keeps its existing error mask.
- Preserved the existing PFlash write/erase paths, read, password, and
  uncompressed-write commands unchanged.
- DFlash command cycles use a fixed per-PMU bank base (`0xAF000000` /
  `0xAF080000`); the DS0/DS1 bank is selected by the page/sector address, not by
  the command base. (Earlier drafts based the command cycles on
  `address & 0xFFFF0000`, which diverged from the ASW for the DS1 region.)
- Rebuilt the root `bootloader.bin` (TriCore GCC 13.4). Its SHA-256 is
  `71402b63af10de819deafd313e15a6a2114baf0ec66a205132cb908c7d84e02e`.

### Verified against the Simos18 ASW (Ghidra)

Cross-checked every value against the Fls flash driver in `8V0906264H__0003`
(SC8O30), which runs on this exact TC1791 silicon:

- Enter Page Mode `0x50`/`0x5D` (`FUN_808814d8`: `type * 0x0D + 0x50`).
- Write Page: AA/55/A0 at `0x5554`/`0xAAA8`, PA←AA, load at `0x55F0`, reset
  `0xF0`; 32 double-words (256 B) for PFlash, 16 (128 B) for DFlash
  (`FUN_80880660`).
- Erase confirm `0x30`/`0x40` (`FUN_80880358`: `type * 0x10 + 0x30`).
- PMU split at `0xAF080000` and command bases `0xAF000000` / `0xAF080000`
  (`FUN_80881538`).
- DFlash extent `0xAF000000` and `0xAF080000`, size `0x18000` each, from the
  driver's block descriptor table at `0x80061ccc`.

### Host tooling

- `compressed_write` now accepts DFlash addresses and paces 128-byte-page
  blocks. Added `resolve_write_page_size()`, `validate_dflash_range()`, and a
  `page_size` argument to `build_compressed_write_metadata()`.
- `erase_sector` now documents and accepts DFlash sector base addresses; the
  target auto-selects `Erase Phys Sector` from the address.

### Notes

- DFlash sectors must be erased before writing. There are two DFlash banks
  (DS0 at `0xAF000000`, DS1 at `0xAF080000`), each a single `0x18000` sector;
  erase the base address of each bank you overwrite. A full ECU hardware DFlash
  erase/write has not yet been validated on real silicon.

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
