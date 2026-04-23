"""
Pre-build script: generate gzipped web assets into data/ for PlatformIO uploadfs.

Steps:
  1. Gzip every file in src/html/ into data/ (keeping directory structure)

Edit src/html/index.html directly — no code generation step.

Firmware upload:   pio run --target upload --upload-port COM38
Filesystem upload: pio run --target uploadfs --upload-port COM38
"""
from SCons.Script import Import
import os
import sys
import gzip
import shutil

Import("env")


def _log(msg):
    print(msg)
    sys.stdout.flush()


def build_web_assets(source, target, env):
    project_dir = env.subst("$PROJECT_DIR")
    src_dir     = os.path.join(project_dir, "src", "html")
    data_dir    = os.path.join(project_dir, "data")

    if not os.path.isdir(src_dir):
        _log("[EPD/FS] src/html/ not found — skip")
        return

    _log("\n" + "=" * 60)
    _log("[EPD/FS] Generating web assets into data/ ...")
    _log("=" * 60)

    # Gzip all files from src/html/ into data/
    os.makedirs(data_dir, exist_ok=True)
    total_raw = 0
    total_gz  = 0
    for dp, _dirs, fnames in os.walk(src_dir):
        for fname in fnames:
            src_path = os.path.join(dp, fname)
            rel_path = os.path.relpath(src_path, src_dir)
            dst_path = os.path.join(data_dir, rel_path + ".gz")
            os.makedirs(os.path.dirname(dst_path), exist_ok=True)
            raw_size = os.path.getsize(src_path)
            with open(src_path, "rb") as fi, \
                 gzip.open(dst_path, "wb", compresslevel=9) as fo:
                fo.write(fi.read())
            gz_size = os.path.getsize(dst_path)
            total_raw += raw_size
            total_gz  += gz_size
            ratio = 100 * (raw_size - gz_size) // raw_size if raw_size else 0
            _log(f"[EPD/FS]   gz: {rel_path} ({raw_size} -> {gz_size} bytes, -{ratio}%)")

    if total_raw:
        ratio = 100 * (total_raw - total_gz) // total_raw
        _log(f"[EPD/FS] Total: {total_raw} -> {total_gz} bytes ({ratio}% saved)")
    _log(f"[EPD/FS] data/ ready. Upload with: pio run --target uploadfs --upload-port <PORT>")


# Run before buildfs (pio run --target buildfs/uploadfs)
env.AddPreAction("$BUILD_DIR/littlefs.bin", build_web_assets)
env.AddPreAction("buildfs",                 build_web_assets)

from SCons.Script import Import, Exit
import os
import sys
import gzip
import shutil
import tempfile
import subprocess

Import("env")


def _log(msg):
    print(msg)
    sys.stdout.flush()


def _find_littlefs_partition(csv_path):
    """Return (offset, size) for the spiffs-subtype data partition, or (None, None)."""
    with open(csv_path, 'r', encoding='utf-8-sig', errors='replace') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = [p.strip() for p in line.split(',')]
            if len(parts) < 5:
                continue
            ptype = parts[1].strip()
            psub  = parts[2].strip()
            if ptype == 'data' and psub == 'spiffs':
                offset = int(parts[3], 0)
                size   = int(parts[4], 0)
                return offset, size
    return None, None


def _find_mklittlefs():
    """Locate mklittlefs binary in PlatformIO packages."""
    pio_home = os.environ.get(
        'PLATFORMIO_HOME_DIR',
        os.path.join(os.path.expanduser('~'), '.platformio'),
    )
    search_roots = [
        os.path.join(pio_home, 'packages'),
        os.path.join(env.subst('$PROJECT_DIR'), '.pio', 'packages'),
    ]
    exe = 'mklittlefs.exe' if sys.platform == 'win32' else 'mklittlefs'
    for pkgs_dir in search_roots:
        if not os.path.isdir(pkgs_dir):
            continue
        for d in sorted(os.listdir(pkgs_dir), reverse=True):
            if d.startswith('tool-mklittlefs'):
                candidate = os.path.join(pkgs_dir, d, exe)
                if os.path.isfile(candidate):
                    return candidate
    return None


# ── Resolve project paths ────────────────────────────────────────────────────
project_dir = env.subst('$PROJECT_DIR')
build_dir   = env.subst('$BUILD_DIR')
data_dir    = os.path.join(project_dir, 'src', 'html')

if not os.path.isdir(data_dir) or not os.listdir(data_dir):
    _log('[EPD/FS] src/html/ not found or empty — skipping LittleFS build')
else:
    # Resolve partition table
    partitions_csv = env.subst('$PARTITIONS_TABLE_CSV')
    if not partitions_csv or not os.path.isfile(partitions_csv):
        try:
            from platformio.project.config import ProjectConfig
            cfg    = ProjectConfig.get_instance()
            pioenv = env.subst('$PIOENV')
            rel    = cfg.get(f'env:{pioenv}', 'board_build.partitions', '')
            if rel:
                partitions_csv = os.path.join(project_dir, rel)
        except Exception:
            pass

    if not partitions_csv or not os.path.isfile(partitions_csv):
        _log('[EPD/FS] Cannot resolve partition table — skipping LittleFS build')
    else:
        fs_start, fs_size = _find_littlefs_partition(partitions_csv)

        if fs_start is None:
            _log(f'[EPD/FS] No spiffs/littlefs partition in {partitions_csv} — skipping')
        else:
            fs_tool = _find_mklittlefs()
            if not fs_tool:
                _log('[EPD/FS] WARNING: mklittlefs not found — skipping LittleFS build')
            else:
                fs_bin   = os.path.join(build_dir, 'littlefs.bin')
                fs_page  = 256
                fs_block = 4096

                _log(f'[EPD/FS] LittleFS auto-build enabled')
                _log(f'[EPD/FS]   partition : {partitions_csv}')
                _log(f'[EPD/FS]   offset    : 0x{fs_start:X}')
                _log(f'[EPD/FS]   size      : 0x{fs_size:X} ({fs_size // 1024} KB)')
                _log(f'[EPD/FS]   tool      : {fs_tool}')

                def build_littlefs(source, target, env):
                    _log('\n' + '=' * 60)
                    _log('[EPD/FS] Building LittleFS image ...')
                    _log('=' * 60)

                    # Gzip all files into a temp directory
                    gz_dir = tempfile.mkdtemp(prefix='epd_fs_gz_')
                    try:
                        total_raw = 0
                        total_gz  = 0
                        for dp, _dirs, fnames in os.walk(data_dir):
                            for fname in fnames:
                                src_path = os.path.join(dp, fname)
                                rel_path = os.path.relpath(src_path, data_dir)
                                dst_path = os.path.join(gz_dir, rel_path + '.gz')
                                os.makedirs(os.path.dirname(dst_path), exist_ok=True)
                                raw_size = os.path.getsize(src_path)
                                with open(src_path, 'rb') as fi, \
                                     gzip.open(dst_path, 'wb', compresslevel=9) as fo:
                                    fo.write(fi.read())
                                gz_size = os.path.getsize(dst_path)
                                total_raw += raw_size
                                total_gz  += gz_size
                                ratio = 100 * (raw_size - gz_size) // raw_size if raw_size else 0
                                _log(f'[EPD/FS]   gz: {rel_path} ({raw_size} → {gz_size} bytes, -{ratio}%)')

                        if total_raw:
                            ratio = 100 * (total_raw - total_gz) // total_raw
                            _log(f'[EPD/FS] Total: {total_raw} → {total_gz} bytes ({ratio}% saved)')

                        # Step 3: run mklittlefs
                        cmd = [
                            fs_tool,
                            '-c', gz_dir,
                            '-p', str(fs_page),
                            '-b', str(fs_block),
                            '-s', str(fs_size),
                            fs_bin,
                        ]
                        _log(f'[EPD/FS] cmd: {" ".join(cmd)}')
                        proc = subprocess.run(cmd, capture_output=True, text=True)
                        combined = (proc.stdout or '') + (proc.stderr or '')
                        for line in combined.strip().splitlines():
                            _log(f'[mklittlefs] {line}')
                        if proc.returncode != 0 or 'error' in combined.lower():
                            _log('[EPD/FS] ERROR: mklittlefs failed!')
                            Exit(1)
                        _log(f'[EPD/FS] LittleFS image → {fs_bin}')
                    finally:
                        shutil.rmtree(gz_dir, ignore_errors=True)

                # Rebuild FS image before every upload
                env.AddPreAction('upload', build_littlefs)

                # Also rebuild after firmware.elf is linked (covers `pio run` without upload)
                env.AddPostAction(
                    '$BUILD_DIR/firmware.elf',
                    env.Action(build_littlefs, '[EPD/FS] Building LittleFS image...'),
                )

                # Tell esptool to flash littlefs.bin at the partition offset
                env.Append(
                    UPLOADERFLAGS=[
                        '0x%X' % fs_start,
                        fs_bin,
                    ]
                )
                _log(f'[EPD/FS] littlefs.bin will be flashed at 0x{fs_start:X} on upload\n')
