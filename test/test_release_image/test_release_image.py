import os
import tempfile
import unittest

from scripts.release_image import (
    build_merge_command,
    default_device_name,
    parse_partition_table,
    release_filename,
)


class ReleaseImageTests(unittest.TestCase):
    def test_partition_offsets_are_parsed_from_csv(self):
        csv = """# Name, Type, SubType, Offset, Size, Flags
nvs, data, nvs, 0x9000, 0x10000,
otadata, data, ota, 0x19000, 0x2000,
app0, app, ota_0, 0x20000, 0x400000,
littlefs, data, spiffs, 0x820000, 0x7E0000,
"""
        with tempfile.NamedTemporaryFile("w", delete=False) as handle:
            handle.write(csv)
            path = handle.name
        try:
            partitions = parse_partition_table(path)
        finally:
            os.unlink(path)

        self.assertEqual(0x19000, partitions["otadata"].offset)
        self.assertEqual(0x20000, partitions["app0"].offset)
        self.assertEqual(0x820000, partitions["littlefs"].offset)

    def test_release_filename_uses_device_and_version(self):
        self.assertEqual("nm-epd-420", default_device_name("nm-display-420"))
        self.assertEqual(
            "esp32-dashboard-nm-epd-420-v0.1.1.bin",
            release_filename("nm-display-420", "v0.1.1"),
        )

    def test_merge_command_sets_chip_before_subcommand(self):
        command = build_merge_command(
            "python",
            "esptool.py",
            "release.bin",
            [(0x0, "bootloader.bin"), (0x20000, "firmware.bin")],
            "esp32s3",
            "dio",
            "80m",
            "16MB",
        )

        self.assertEqual("python", command[0])
        self.assertEqual("esptool.py", command[1])
        self.assertEqual(["--chip", "esp32s3", "merge-bin"], command[2:5])
        self.assertIn("0x20000", command)


if __name__ == "__main__":
    unittest.main()
