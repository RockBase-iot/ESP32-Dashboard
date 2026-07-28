"""
Generate gzipped web assets for PlatformIO LittleFS uploads.

Normal NM-EPD-420 flashing should use:
    pio run -e nm-display-420 -t upload_all --upload-port <PORT>

The upload_all target uploads firmware first, then uploads the LittleFS image
that contains the web portal assets.
"""

from SCons.Script import AlwaysBuild, Import
import gzip
import os
import subprocess
import sys

Import("env")

sys.path.insert(0, env.subst("$PROJECT_DIR"))
from scripts.release_image import create_release_image


def _log(message):
    print(message)
    sys.stdout.flush()


def build_web_assets(source, target, env):
    project_dir = env.subst("$PROJECT_DIR")
    src_dir = os.path.join(project_dir, "src", "html")
    data_dir = os.path.join(project_dir, "data")

    if not os.path.isdir(src_dir):
        _log("[EPD/FS] src/html/ not found; skip web asset generation")
        return

    _log("")
    _log("=" * 60)
    _log("[EPD/FS] Generating gzipped web assets into data/")
    _log("=" * 60)

    os.makedirs(data_dir, exist_ok=True)
    total_raw = 0
    total_gz = 0
    generated = 0

    for root, _dirs, filenames in os.walk(src_dir):
        for filename in filenames:
            src_path = os.path.join(root, filename)
            rel_path = os.path.relpath(src_path, src_dir)
            dst_path = os.path.join(data_dir, rel_path + ".gz")
            os.makedirs(os.path.dirname(dst_path), exist_ok=True)

            raw_size = os.path.getsize(src_path)
            with open(src_path, "rb") as src_file, gzip.open(dst_path, "wb", compresslevel=9) as gz_file:
                gz_file.write(src_file.read())
            gz_size = os.path.getsize(dst_path)

            total_raw += raw_size
            total_gz += gz_size
            generated += 1
            ratio = 100 * (raw_size - gz_size) // raw_size if raw_size else 0
            _log(f"[EPD/FS]   gz: {rel_path} ({raw_size} -> {gz_size} bytes, -{ratio}%)")

    if generated == 0:
        _log("[EPD/FS] src/html/ is empty; LittleFS web portal will be empty")
        return

    ratio = 100 * (total_raw - total_gz) // total_raw if total_raw else 0
    _log(f"[EPD/FS] Total: {total_raw} -> {total_gz} bytes ({ratio}% saved)")
    _log("[EPD/FS] data/ ready for PlatformIO buildfs/uploadfs")


env.AddPreAction("$BUILD_DIR/littlefs.bin", build_web_assets)

def upload_all_action(target, source, env):
    project_dir = env.subst("$PROJECT_DIR")
    pioenv = env.subst("$PIOENV")
    upload_port = env.subst("$UPLOAD_PORT")
    base_cmd = [sys.executable, "-m", "platformio", "run", "-e", pioenv]
    if upload_port:
        base_cmd.extend(["--upload-port", upload_port])

    for pio_target in ("upload", "uploadfs"):
        cmd = base_cmd + ["-t", pio_target]
        _log(f"[EPD/FS] Running serial target: {' '.join(cmd)}")
        child_env = os.environ.copy()
        child_env.setdefault("PYTHONIOENCODING", "utf-8")
        child_env.setdefault("PYTHONUTF8", "1")
        result = subprocess.call(cmd, cwd=project_dir, env=child_env)
        if result != 0:
            _log(f"[EPD/FS] Target {pio_target} failed with exit code {result}")
            return result
    return 0


upload_all = env.Alias("upload_all", [], env.Action(upload_all_action, "[EPD/FS] Upload firmware + LittleFS"))
AlwaysBuild(upload_all)


def release_bin_action(target, source, env):
    project_dir = env.subst("$PROJECT_DIR")
    build_dir = env.subst("$BUILD_DIR")
    pioenv = env.subst("$PIOENV")
    platform = env.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif32")
    esptool_dir = platform.get_package_dir("tool-esptoolpy")
    partition_csv = env.subst("$BOARD_FCPATH")
    if not partition_csv or not os.path.isfile(partition_csv):
        partition_csv = os.path.join(project_dir, "partition", "partitions_16mb_ota.csv")

    def env_value(name, default):
        value = env.subst(name)
        if not value or value == name or value.startswith("$"):
            return default
        return value

    def flash_freq_value():
        value = env_value("$BOARD_F_FLASH", "80m")
        normalized = value.strip().lower().rstrip("l")
        if normalized in ("80000000", "80"):
            return "80m"
        if normalized in ("40000000", "40"):
            return "40m"
        return value

    output_path = create_release_image(
        project_dir=project_dir,
        pioenv=pioenv,
        build_dir=build_dir,
        partition_csv=partition_csv,
        framework_dir=framework_dir,
        tool_esptoolpy_dir=esptool_dir,
        flash_mode=env_value("$BOARD_FLASH_MODE", "dio"),
        flash_freq=flash_freq_value(),
        flash_size=env_value("$BOARD_UPLOAD_FLASH_SIZE", "16MB"),
        chip=env_value("$BOARD_MCU", "esp32s3"),
        python_exe=sys.executable,
    )
    _log(f"[EPD/Release] Created 0x0 flash image: {output_path}")
    return 0


release_bin = env.Alias(
    "release_bin",
    ["$BUILD_DIR/${PROGNAME}.bin", "$BUILD_DIR/littlefs.bin"],
    env.Action(release_bin_action, "[EPD/Release] Merge firmware + LittleFS release image"),
)
AlwaysBuild(release_bin)
