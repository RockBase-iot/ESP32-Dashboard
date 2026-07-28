from __future__ import annotations

from dataclasses import dataclass
import os
import subprocess
import sys
from typing import Dict, List, Optional, Sequence, Tuple


@dataclass(frozen=True)
class Partition:
    name: str
    type: str
    subtype: str
    offset: int
    size: int


DEVICE_NAME_BY_ENV = {
    "nm-display-420": "nm-epd-420",
}


def _parse_int(value: str) -> int:
    value = value.strip()
    return int(value, 16) if value.lower().startswith("0x") else int(value)


def parse_partition_table(path: str) -> Dict[str, Partition]:
    partitions: Dict[str, Partition] = {}
    with open(path, "r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.split("#", 1)[0].strip()
            if not line:
                continue
            columns = [column.strip() for column in line.split(",")]
            if len(columns) < 5 or columns[0].lower() == "name":
                continue
            partition = Partition(
                name=columns[0],
                type=columns[1],
                subtype=columns[2],
                offset=_parse_int(columns[3]),
                size=_parse_int(columns[4]),
            )
            partitions[partition.name] = partition
    return partitions


def default_device_name(pioenv: str) -> str:
    if pioenv in DEVICE_NAME_BY_ENV:
        return DEVICE_NAME_BY_ENV[pioenv]
    return pioenv.replace("_", "-")


def normalized_version(version: str) -> str:
    version = version.strip()
    if not version:
        raise ValueError("release version is empty")
    return version if version.startswith("v") else f"v{version}"


def release_filename(pioenv: str, version: str) -> str:
    device = default_device_name(pioenv)
    return f"esp32-dashboard-{device}-{normalized_version(version)}.bin"


def read_release_version(project_dir: str) -> str:
    version_path = os.path.join(project_dir, "VERSION")
    if os.path.isfile(version_path):
        with open(version_path, "r", encoding="utf-8") as handle:
            return normalized_version(handle.readline())
    return "v0.1.1"


def find_file(root: Optional[str], filename: str) -> Optional[str]:
    if not root or not os.path.isdir(root):
        return None
    for current_root, _dirs, files in os.walk(root):
        if filename in files:
            return os.path.join(current_root, filename)
    return None


def build_segments(
    build_dir: str,
    partition_csv: str,
    framework_dir: Optional[str],
) -> List[Tuple[int, str]]:
    partitions = parse_partition_table(partition_csv)
    app_partition = partitions.get("app0")
    fs_partition = partitions.get("littlefs")
    ota_partition = partitions.get("otadata")
    if app_partition is None:
        raise ValueError(f"{partition_csv} does not contain an app0 partition")
    if fs_partition is None:
        raise ValueError(f"{partition_csv} does not contain a littlefs partition")

    required = [
        (0x0, os.path.join(build_dir, "bootloader.bin")),
        (0x8000, os.path.join(build_dir, "partitions.bin")),
        (app_partition.offset, os.path.join(build_dir, "firmware.bin")),
        (fs_partition.offset, os.path.join(build_dir, "littlefs.bin")),
    ]
    for _offset, path in required:
        if not os.path.isfile(path):
            raise FileNotFoundError(path)

    segments: List[Tuple[int, str]] = required
    boot_app0 = find_file(framework_dir, "boot_app0.bin")
    if ota_partition is not None and boot_app0:
        segments.insert(2, (ota_partition.offset, boot_app0))
    return segments


def _format_address(offset: int) -> str:
    return f"0x{offset:x}"


def build_merge_command(
    python_exe: str,
    esptool_py: str,
    output_path: str,
    segments: Sequence[Tuple[int, str]],
    chip: str,
    flash_mode: str,
    flash_freq: str,
    flash_size: str,
) -> List[str]:
    command = [
        python_exe,
        esptool_py,
        "--chip",
        chip,
        "merge-bin",
        "-o",
        output_path,
        "--flash-mode",
        flash_mode,
        "--flash-freq",
        flash_freq,
        "--flash-size",
        flash_size,
        "--format",
        "raw",
    ]
    for offset, path in sorted(segments, key=lambda item: item[0]):
        command.extend([_format_address(offset), path])
    return command


def find_esptool_py(tool_esptoolpy_dir: Optional[str]) -> str:
    candidate = os.path.join(tool_esptoolpy_dir or "", "esptool.py")
    if os.path.isfile(candidate):
        return candidate
    raise FileNotFoundError("Could not locate tool-esptoolpy/esptool.py")


def release_output_path(project_dir: str, pioenv: str, version: str) -> str:
    return os.path.join(project_dir, "release", release_filename(pioenv, version))


def create_release_image(
    *,
    project_dir: str,
    pioenv: str,
    build_dir: str,
    partition_csv: str,
    framework_dir: Optional[str],
    tool_esptoolpy_dir: Optional[str],
    flash_mode: str = "dio",
    flash_freq: str = "80m",
    flash_size: str = "16MB",
    chip: str = "esp32s3",
    python_exe: str = sys.executable,
) -> str:
    version = read_release_version(project_dir)
    output_path = release_output_path(project_dir, pioenv, version)
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    segments = build_segments(build_dir, partition_csv, framework_dir)
    esptool_py = find_esptool_py(tool_esptoolpy_dir)
    command = build_merge_command(
        python_exe,
        esptool_py,
        output_path,
        segments,
        chip,
        flash_mode,
        flash_freq,
        flash_size,
    )
    subprocess.check_call(command, cwd=project_dir)
    return output_path
