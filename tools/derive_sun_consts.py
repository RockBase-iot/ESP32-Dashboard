"""Derive radius/centre constants that match the artwork at every icon size.

Measured on the 96x96 bitmap (scale = 3):
    disc rows 38..54  -> centre y = 46.5, radius ~ 11
    disc x    37..58  -> centre x = 47.5, radius ~ 11

Expressed as multiples of `scale` (scale = size/32):
    radius  11 / 3 = 3.67
    cx      47.5 / 3 = 15.83
    cy      46.5 / 3 = 15.50

We check that the *relative* geometry holds for 96, 32 and 24 px variants by
measuring each bitmap, so one constant set works everywhere.
"""
import re
import os

BASE = r"E:\github\ESP32-Dashboard\src\assets\icons"
ICONS = [
    (96, "wi_day_sunny_96x96.h"),
    (32, "wi_day_sunny_32x32.h"),
    (24, "wi_day_sunny_24x24.h"),
    (16, "wi_day_sunny_16x16.h"),
]


def load_bits(path, size):
    src = open(path, encoding="utf-8", errors="replace").read()
    body = src[src.find("{"):]
    vals = [int(m, 16) for m in re.findall(r"0[xX]([0-9a-fA-F]{2})", body)]
    bpr = (size + 7) // 8
    need = size * bpr
    if len(vals) < need:
        return None
    return [[(vals[r * bpr + c // 8] >> (7 - (c & 7))) & 1 for c in range(size)]
            for r in range(size)]


def enclosed_runs(row):
    out = []
    start = None
    for x, v in enumerate(row):
        if v == 1:
            if start is None:
                start = x
        else:
            if start is not None and start > 0:
                out.append((start, x - 1, x - start))
            start = None
    return out


print(f"{'size':>5} {'scale':>6} {'disc rows':>12} {'r(px)':>6} "
      f"{'cx/scale':>9} {'cy/scale':>9} {'r/scale':>8}")

results = {}
for size, fname in ICONS:
    path = os.path.join(BASE, f"{size}x{size}", fname)
    if not os.path.isfile(path):
        print(f"{size:>5}   (missing {fname})")
        continue
    bits = load_bits(path, size)
    if bits is None:
        print(f"{size:>5}   (short array)")
        continue

    # search the middle 60% of rows for the widest enclosed transparent run
    lo, hi = int(size * 0.20), int(size * 0.85)
    found = []
    for y in range(lo, hi):
        for (s, e, w) in enclosed_runs(bits[y]):
            if w >= max(5, size // 8):
                found.append((y, s, e, w))
    if not found:
        print(f"{size:>5}   (no disc found)")
        continue

    # drop the long thin ray-gap rows: keep rows within +-25% of the widest
    wmax = max(d[3] for d in found)
    disc = [d for d in found if d[3] >= wmax * 0.75]
    ymin, ymax = disc[0][0], disc[-1][0]
    maxrow = max(disc, key=lambda d: d[3])
    cx = (maxrow[1] + maxrow[2]) / 2.0
    cy = (ymin + ymax) / 2.0
    r = wmax / 2.0
    scale = size / 32.0
    results[size] = (cx, cy, r)
    print(f"{size:>5} {scale:>6.2f} {f'{ymin}..{ymax}':>12} {r:>6.1f} "
          f"{cx/scale:>9.2f} {cy/scale:>9.2f} {r/scale:>8.2f}")

print()
if results:
    # propose constants: use the 96px measurement as the reference, and verify
    # the same ratios are sane at the other sizes
    print("proposed constants (as multiples of scale = size/32):")
    xs = [v[0] / (k / 32.0) for k, v in results.items()]
    ys = [v[1] / (k / 32.0) for k, v in results.items()]
    rs = [v[2] / (k / 32.0) for k, v in results.items()]
    print(f"  kSunCenterXRatio = {sum(xs)/len(xs):.2f}   (range {min(xs):.2f}..{max(xs):.2f})")
    print(f"  kSunCenterYRatio = {sum(ys)/len(ys):.2f}   (range {min(ys):.2f}..{max(ys):.2f})")
    print(f"  kSunRadiusRatio  = {sum(rs)/len(rs):.2f}   (range {min(rs):.2f}..{max(rs):.2f})")
    print()
    print("current code uses: centre (16.00, 15.00)*scale, radius 7.00*scale")
