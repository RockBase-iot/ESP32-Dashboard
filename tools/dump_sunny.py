import re
import sys
from pathlib import Path

ROOT = Path("E:/github/ESP32-Dashboard")
C_ARRAY = re.compile(
    r"(?:static\s+)?(?:const\s+)?(?:unsigned\s+char|uint8_t)\s+(\w+)\s*\[\s*\]\s*"
    r"(?:PROGMEM\s*)?=\s*\{(.*?)\};",
    re.S,
)

name = sys.argv[1] if len(sys.argv) > 1 else "wi_day_sunny_96x96"
size = int(sys.argv[2]) if len(sys.argv) > 2 else 96

vals = None
for f in (ROOT / "src" / "assets" / "icons").rglob("*.h"):
    d = {n: [int(v, 0) for v in re.findall(r"0[xX][0-9a-fA-F]+|\d+", b)]
         for n, b in C_ARRAY.findall(f.read_text(errors="replace"))}
    if name in d:
        vals = d[name]
        print(f"found {name} in {f.name}: {len(vals)} bytes "
              f"(expect {(size + 7) // 8 * size})")
        break

if vals is None:
    sys.exit("not found")

bpr = (size + 7) // 8
print(f"bytes/row = {bpr}, computed rows = {len(vals) / bpr:.2f}")
for r in range(min(size, 48)):
    line = ""
    for c in range(size):
        b = vals[r * bpr + (c >> 3)]
        line += "#" if (b & (0x80 >> (c & 7))) else "."
    print(f"{r:3d} {line}")
