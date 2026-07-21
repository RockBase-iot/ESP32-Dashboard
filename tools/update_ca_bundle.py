#!/usr/bin/env python3
"""Generate the committed Mozilla CA bundle for NM-EPD-420 calendar HTTPS.

The firmware must use certificate verification and must not call insecure TLS
bypass APIs.
This script uses Espressif's official ``gen_crt_bundle.py`` against the Mozilla
``cacrt_all.pem`` bundled with the local ESP-IDF package.
"""

from __future__ import annotations

import argparse
import pathlib
import shutil
import subprocess
import sys

DEFAULT_OUTPUT = pathlib.Path("src/assets/certs/mozilla_crt_bundle.bin")
DEFAULT_ESPIDF_ROOT = pathlib.Path.home() / ".platformio/packages/framework-espidf"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=pathlib.Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--espidf-root", type=pathlib.Path, default=DEFAULT_ESPIDF_ROOT)
    parser.add_argument("--max-certs", type=int, default=200)
    args = parser.parse_args()

    bundle_dir = args.espidf_root / "components/mbedtls/esp_crt_bundle"
    generator = bundle_dir / "gen_crt_bundle.py"
    ca_pem = bundle_dir / "cacrt_all.pem"
    if not generator.exists() or not ca_pem.exists():
        raise SystemExit(f"ESP-IDF certificate bundle tools not found under {bundle_dir}")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    workdir = args.output.parent
    generated = workdir / "x509_crt_bundle"
    if generated.exists():
        generated.unlink()
    subprocess.run(
        [
            sys.executable,
            str(generator),
            "--quiet",
            "--input",
            str(ca_pem),
            "--max-certs",
            str(args.max_certs),
        ],
        cwd=workdir,
        check=True,
    )
    shutil.move(str(generated), args.output)
    print(f"wrote {args.output} from {ca_pem}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
