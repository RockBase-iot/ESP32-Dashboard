"""Measure the sun disc in vi_day_sunny_96x96 by scanning only the central band.

Rows near the top/bottom of the bitmap are fully transparent (empty margin),
which defeats a naive "longest transparent run" scan. The disc lives roughly
in rows 30..62, so we restrict the search there and require the run to be
bounded by black ink on BOTH sides (i.e. not touching the row edges).
"""
import re

ICON = r"E:\github\ESP32-Dashboard\src\assets\icons\96x96\wi_day_sunny_96x96.h"
SIZE = 96


def load_bits(path, size):
    src = open(path, encoding="utf-8", errors="replace").read()
    body = src[src.find("{"):]
    vals = [int(m, 16) for m in re.findall(r"0[xX]([0-9a-fA-F]{2})", body)]
    bpr = (size + 7) // 8
    return [[(vals[r * bpr + c // 8] >> (7 - (c & 7))) & 1 for c in range(size)]
            for r in range(size)]


def enclosed_runs(row):
    """Transparent runs on this row that have black ink on both sides."""
    out = []
    start = None
    for x, v in enumerate(row):
        if v == 1:
            if start is None:
                start = x
        else:
            if start is not None:
                if start > 0:  # has black ink to the left
                    out.append((start, x - 1, x - start))
                start = None
    # a run reaching the right edge is not enclosed
    return out


bits = load_bits(ICON, SIZE)

# Restrict to the vertical band that actually contains the disc.
LO, HI = 28, 66
found = []
for y in range(LO, HI):
    for (s, e, w) in enclosed_runs(bits[y]):
        if w >= 15:
            found.append((y, s, e, w))

print("enclosed transparent runs (width>=15) in rows", LO, "..", HI)
for (y, s, e, w) in found:
    print(f"  y={y:3}  x {s:3}..{e:3}  width {w:3}")

if found:
    ymin, ymax = found[0][0], found[-1][0]
    maxrow = max(found, key=lambda d: d[3])
    cx = (maxrow[1] + maxrow[2]) / 2.0
    cy = (ymin + ymax) / 2.0
    maxr = max(d[3] for d in found) / 2.0
    print()
    print(f"disc rows {ymin}..{ymax} (height {ymax - ymin + 1})")
    print(f"widest at y={maxrow[0]}: width {maxrow[3]}")
    print(f"disc centre = ({cx:.1f}, {cy:.1f})   radius = {maxr:.1f}")
    print()
    print("drawYellowSun() at scale=3 -> centre (48, 45), radius 21")
    print(f"OFFSET dx = {cx - 48:+.1f}   dy = {cy - 45:+.1f}   radius err = {maxr - 21:+.1f}")
