"""
merge_firmware.py — PlatformIO extra_script (post-build hook)

After firmware.bin and littlefs.bin are compiled, merge:
  bootloader + partitions + boot_app0 + firmware + littlefs
into a single all-in-one binary at the project root:
    esp32-dashboard.bin  (flash from address 0x0)

Usage — referenced in platformio.ini:
    extra_scripts = post:scripts/merge_firmware.py
"""

import os
import subprocess
import sys

Import("env")   # noqa: F821  — SCons environment injected by PlatformIO


def _find_boot_app0(env):
    """Locate boot_app0.bin in the framework package."""
    framework_dir = env.subst("$PROJECT_PACKAGES_DIR")

    # Try the standard arduino-esp32 location first
    for entry in os.listdir(framework_dir):
        if "arduinoespressif32" in entry.lower():
            candidate = os.path.join(
                framework_dir, entry, "tools", "partitions", "boot_app0.bin"
            )
            if os.path.isfile(candidate):
                return candidate

    # Fallback: search platformio home
    pio_home = os.path.expanduser("~/.platformio/packages")
    if os.path.isdir(pio_home):
        for entry in os.listdir(pio_home):
            if "arduinoespressif32" in entry.lower():
                candidate = os.path.join(
                    pio_home, entry, "tools", "partitions", "boot_app0.bin"
                )
                if os.path.isfile(candidate):
                    return candidate

    return None


def _merge_firmware(source, target, env):   # noqa: ANN001
    """Post-build hook: merge all binaries into a single esp32-dashboard.bin."""

    build_dir   = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    output      = os.path.join(project_dir, "esp32-dashboard.bin")

    bootloader = os.path.join(build_dir, "bootloader.bin")
    partitions = os.path.join(build_dir, "partitions.bin")
    firmware   = os.path.join(build_dir, "firmware.bin")
    littlefs   = os.path.join(build_dir, "littlefs.bin")
    boot_app0  = _find_boot_app0(env)

    # Verify required files exist
    for f in (bootloader, partitions, firmware):
        if not os.path.isfile(f):
            print("[merge] WARNING: %s not found, skipping merge." % f)
            return
    if not boot_app0 or not os.path.isfile(boot_app0):
        print("[merge] WARNING: boot_app0.bin not found, skipping merge.")
        return

    # Check if littlefs.bin exists (only if filesystem was built)
    has_fs = os.path.isfile(littlefs)
    if not has_fs:
        print("[merge] NOTE: littlefs.bin not found, merging without filesystem.")

    # Locate esptool.py
    esptool = os.path.join(
        env.subst("$PROJECT_PACKAGES_DIR"),
        "tool-esptoolpy", "esptool.py"
    )
    if not os.path.isfile(esptool):
        esptool_cmd = [sys.executable, "-m", "esptool"]
    else:
        esptool_cmd = [sys.executable, esptool]

    args = esptool_cmd + [
        "--chip", "esp32s3",
        "merge_bin",
        "-o", output,
        "--flash_mode", "dio",
        "--flash_freq", "80m",
        "--flash_size", "16MB",
        "0x0000", bootloader,
        "0x8000", partitions,
        "0xe000", boot_app0,
        "0x10000", firmware,
    ]

    # Append littlefs at its partition offset if available
    if has_fs:
        args += ["0x810000", littlefs]

    print()
    print("=" * 60)
    print("[merge] Creating esp32-dashboard.bin")
    print("[merge]   bootloader  @ 0x0000")
    print("[merge]   partitions  @ 0x8000")
    print("[merge]   boot_app0   @ 0xe000")
    print("[merge]   firmware    @ 0x10000")
    if has_fs:
        print("[merge]   littlefs    @ 0x810000")
    print("=" * 60)

    result = subprocess.run(args, cwd=project_dir)

    if result.returncode == 0:
        size_mb = os.path.getsize(output) / (1024 * 1024)
        print("[merge] OK -> esp32-dashboard.bin  (%.1f MB)" % size_mb)
        print("[merge] Flash from address 0x0 to programme the whole device.")
    else:
        print("[merge] ERROR: merge_bin exited with code %d" % result.returncode)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _merge_firmware)
