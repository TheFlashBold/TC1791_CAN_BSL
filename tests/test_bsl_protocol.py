import unittest

from bsl_protocol import (
    WRITE_BLOCK_DONE,
    WRITE_BLOCK_SIZE,
    WRITE_COMPRESSED,
    WRITE_COMPRESSED_BOUND,
    WRITE_READY,
    CompressedWriteProtocolError,
    build_compressed_write_metadata,
    build_compressed_write_start,
    iter_compressed_write_data_frames,
    parse_compressed_write_response,
    resolve_write_page_size,
    validate_dflash_range,
    validate_pflash_range,
)


class CompressedWriteProtocolTests(unittest.TestCase):
    def test_start_frame_uses_big_endian_address_and_24_bit_length(self):
        self.assertEqual(
            build_compressed_write_start(0x80820000, 0x09FC00),
            bytes.fromhex("08 80 82 00 00 09 fc 00"),
        )

    def test_range_must_be_page_aligned_and_stay_in_one_bank(self):
        for address, length in (
            (0x80820001, 0x100),
            (0x80820000, 0x101),
            (0x801FFF00, 0x200),
            (0x80200000, 0x100),
        ):
            with self.subTest(address=address, length=length), self.assertRaises(
                ValueError
            ):
                validate_pflash_range(address, length)

    def test_page_size_is_resolved_from_the_target_array(self):
        self.assertEqual(resolve_write_page_size(0x80000000, 0x200000), 256)
        self.assertEqual(resolve_write_page_size(0x80800000, 0x100000), 256)
        self.assertEqual(resolve_write_page_size(0xAF000000, 0x18000), 128)
        self.assertEqual(resolve_write_page_size(0xAF080000, 0x18000), 128)
        for address, length in (
            (0xAF000001, 0x180),  # not 128-aligned
            (0xAF000000, 0x18080),  # overruns the bank
            (0xAF07FF80, 0x100),  # outside any bank
            (0xAE000000, 0x100),  # not a flash array
        ):
            with self.subTest(address=address, length=length), self.assertRaises(
                ValueError
            ):
                resolve_write_page_size(address, length)

    def test_dflash_start_frame_and_range_validation(self):
        self.assertEqual(
            build_compressed_write_start(0xAF000000, 0x18000),
            bytes.fromhex("08 af 00 00 00 01 80 00"),
        )
        validate_dflash_range(0xAF080000, 0x18000)
        with self.assertRaises(ValueError):
            validate_dflash_range(0xAF000040, 0x80)

    def test_dflash_metadata_accepts_128_byte_pages(self):
        self.assertEqual(
            build_compressed_write_metadata(0x80, 0x40, 128),
            bytes.fromhex("08 ff 00 80 00 40 00 00"),
        )
        with self.assertRaises(ValueError):
            build_compressed_write_metadata(0x80, 0x40, 256)

    def test_metadata_contains_raw_and_compressed_lengths(self):
        self.assertEqual(
            build_compressed_write_metadata(WRITE_BLOCK_SIZE, 0x321),
            bytes.fromhex("08 ff 10 00 03 21 00 00"),
        )
        with self.assertRaises(ValueError):
            build_compressed_write_metadata(255, 100)
        with self.assertRaises(ValueError):
            build_compressed_write_metadata(WRITE_BLOCK_SIZE, WRITE_COMPRESSED_BOUND + 1)

    def test_data_frames_are_sequenced_and_last_frame_is_padded(self):
        frames = list(iter_compressed_write_data_frames(bytes(range(14))))
        self.assertEqual(
            frames,
            [
                bytes.fromhex("08 00 00 01 02 03 04 05"),
                bytes.fromhex("08 01 06 07 08 09 0a 0b"),
                bytes.fromhex("08 02 0c 0d aa aa aa aa"),
            ],
        )

    def test_sequence_wraps_without_colliding_with_command_state(self):
        frames = list(iter_compressed_write_data_frames(bytes(WRITE_COMPRESSED_BOUND)))
        self.assertEqual(frames[0][1], 0)
        self.assertEqual(frames[255][1], 255)
        self.assertEqual(frames[256][1], 0)
        self.assertTrue(all(frame[0] == WRITE_COMPRESSED for frame in frames))

    def test_response_parser_returns_final_flag_and_fsr(self):
        final, fsr = parse_compressed_write_response(
            bytes.fromhex("08 a2 00 01 12 34 56 78"), WRITE_BLOCK_DONE
        )
        self.assertTrue(final)
        self.assertEqual(fsr, 0x12345678)

    def test_response_parser_surfaces_target_errors(self):
        with self.assertRaisesRegex(CompressedWriteProtocolError, "decompression"):
            parse_compressed_write_response(
                bytes.fromhex("08 a0 04 00 00 00 00 00"), WRITE_READY
            )


if __name__ == "__main__":
    unittest.main()
