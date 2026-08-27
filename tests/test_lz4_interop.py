import ctypes
import random
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import lz4.block


class TargetLz4InteropTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which("cc")
        if compiler is None:
            raise unittest.SkipTest("a native C compiler is required")
        cls.temp_dir = tempfile.TemporaryDirectory()
        library_name = "libtarget_lz4.dylib" if sys.platform == "darwin" else "libtarget_lz4.so"
        cls.library_path = Path(cls.temp_dir.name) / library_name
        source = (
            Path(__file__).parents[1]
            / "Bootloader_2"
            / "src"
            / "lz4_decompress.c"
        )
        shared_flag = "-dynamiclib" if sys.platform == "darwin" else "-shared"
        subprocess.run(
            [
                compiler,
                shared_flag,
                "-fPIC",
                "-O2",
                f"-I{source.parent}",
                str(source),
                "-o",
                str(cls.library_path),
            ],
            check=True,
            capture_output=True,
        )
        cls.library = ctypes.CDLL(str(cls.library_path))
        cls.decompress = cls.library.LZ4_decompress_safe
        cls.decompress.argtypes = [
            ctypes.c_void_p,
            ctypes.c_void_p,
            ctypes.c_int,
            ctypes.c_int,
        ]
        cls.decompress.restype = ctypes.c_int

    @classmethod
    def tearDownClass(cls):
        cls.temp_dir.cleanup()

    def assert_roundtrip(self, raw):
        compressed = lz4.block.compress(raw, store_size=False)
        self.assertLessEqual(len(compressed), 4128)
        source = ctypes.create_string_buffer(compressed)
        destination = ctypes.create_string_buffer(len(raw))
        decoded_length = self.decompress(
            source, destination, len(compressed), len(raw)
        )
        self.assertEqual(decoded_length, len(raw))
        self.assertEqual(destination.raw, raw)

    def test_python_blocks_decode_with_target_c_implementation(self):
        fixtures = (
            bytes(256),
            bytes([0xFF]) * 4096,
            bytes(range(256)) * 16,
            bytes((index * 73 + 19) & 0xFF for index in range(4096)),
            random.Random(0x1791).randbytes(4096),
        )
        for raw in fixtures:
            with self.subTest(length=len(raw), prefix=raw[:4].hex()):
                self.assert_roundtrip(raw)

    def test_malformed_block_is_rejected(self):
        source = ctypes.create_string_buffer(b"\x00\x00\x00")
        destination = ctypes.create_string_buffer(4096)
        self.assertLess(self.decompress(source, destination, 3, 4096), 0)


if __name__ == "__main__":
    unittest.main()
