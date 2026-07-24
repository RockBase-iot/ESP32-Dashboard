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
