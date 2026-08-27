"""Pure helpers for the TC1791 CAN BSL compressed-write protocol."""

from collections.abc import Iterator

WRITE_COMPRESSED = 0x08

WRITE_READY = 0xA0
WRITE_BLOCK_READY = 0xA1
WRITE_BLOCK_DONE = 0xA2

WRITE_PAGE_SIZE = 256
WRITE_BLOCK_SIZE = 4096
WRITE_COMPRESSED_BOUND = WRITE_BLOCK_SIZE + (WRITE_BLOCK_SIZE // 255) + 16

PFLASH_RANGES = (
    (0x80000000, 0x80200000),
    (0x80800000, 0x80A00000),
)

WRITE_ERRORS = {
    0x01: "invalid address or total length",
    0x02: "invalid compressed block metadata",
    0x03: "CAN frame sequence mismatch",
    0x04: "LZ4 decompression failed or produced the wrong size",
    0x05: "flash controller reported a programming error",
    0x06: "unexpected frame for the current write state",
}


class CompressedWriteProtocolError(RuntimeError):
    """Raised when a compressed-write request or response is malformed."""


def validate_pflash_range(address: int, length: int) -> None:
    if address % WRITE_PAGE_SIZE or length <= 0 or length % WRITE_PAGE_SIZE:
        raise ValueError("address and length must be aligned to 256-byte pages")
    if length > 0xFFFFFF:
        raise ValueError("length exceeds the 24-bit protocol field")
    if not any(start <= address and length <= end - address for start, end in PFLASH_RANGES):
        raise ValueError("write range must stay inside one TC1791 PFlash bank")


def build_compressed_write_start(address: int, length: int) -> bytes:
    validate_pflash_range(address, length)
    return bytes([WRITE_COMPRESSED]) + address.to_bytes(4, "big") + length.to_bytes(
        3, "big"
    )


def build_compressed_write_metadata(raw_length: int, compressed_length: int) -> bytes:
    if (
        raw_length <= 0
        or raw_length > WRITE_BLOCK_SIZE
        or raw_length % WRITE_PAGE_SIZE
    ):
        raise ValueError("raw block length must be a page-aligned value up to 4096")
    if compressed_length <= 0 or compressed_length > WRITE_COMPRESSED_BOUND:
        raise ValueError("compressed block exceeds the target buffer")
    return bytes(
        [
            WRITE_COMPRESSED,
            0xFF,
            (raw_length >> 8) & 0xFF,
            raw_length & 0xFF,
            (compressed_length >> 8) & 0xFF,
            compressed_length & 0xFF,
            0,
            0,
        ]
    )


def iter_compressed_write_data_frames(compressed: bytes) -> Iterator[bytes]:
    if not compressed or len(compressed) > WRITE_COMPRESSED_BOUND:
        raise ValueError("compressed payload has an invalid length")
    sequence = 0
    for offset in range(0, len(compressed), 6):
        payload = compressed[offset : offset + 6]
        yield bytes([WRITE_COMPRESSED, sequence]) + payload.ljust(6, b"\xAA")
        sequence = (sequence + 1) & 0xFF


def parse_compressed_write_response(
    payload: bytes, expected_stage: int
) -> tuple[bool, int]:
    if len(payload) != 8:
        raise CompressedWriteProtocolError("compressed-write response must be 8 bytes")
    if payload[0] != WRITE_COMPRESSED:
        raise CompressedWriteProtocolError("response belongs to a different command")
    if payload[1] != expected_stage:
        raise CompressedWriteProtocolError(
            f"unexpected response stage 0x{payload[1]:02x}; "
            f"expected 0x{expected_stage:02x}"
        )
    if payload[2]:
        description = WRITE_ERRORS.get(payload[2], "unknown target error")
        raise CompressedWriteProtocolError(
            f"target rejected compressed write: {description} (0x{payload[2]:02x})"
        )
    return bool(payload[3]), int.from_bytes(payload[4:8], "big")
