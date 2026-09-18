"""Render the final proposed sun overlay and compare with the current one.

Constants measured consistently across the 96/32/24/16 px bitmaps:
    centre = (15.4, 15.2) * scale     (current: 16.0, 15.0)
    radius =  6.2         * scale     (current: 7.0)

The disc must stay strictly inside the bitmap's black ring; an oversized disc
paints yellow over the ring and the rays, which is what made the icon look
blurred and clumsy on the panel.
"""
import re

ICON = r"E:\github\ESP32-Dashboard\src\assets\icons\96x96\wi_day_sunny_96x96.h"
SIZE = 96
TRANS, YELLOW, BLACK = " ", "Y", "#"


def load_bits(path, size):
    src = open(path, encoding="utf-8", errors="replace").read()
    body = src[src.find("{"):]
    vals = [int(m, 16) for m in re.findall(r"0[xX]([0-9a-fA-F]{2})", body)]
    bpr = (size + 7) // 8
    return [[(vals[r * bpr + c // 8] >> (7 - (c & 7))) & 1 for c in range(size)]
            for r in range(size)]


BITS = load_bits(ICON, SIZE)


def composite(scale, cx_r, cy_r, r_r):
    cx = int(round(cx_r * scale))
    cy = int(round(cy_r * scale))
    r = int(round(r_r * scale))
    canvas = [[TRANS] * SIZE for _ in range(SIZE)]
    for x in range(SIZE):
        for y in range(SIZE):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                canvas[y][x] = YELLOW
    for y in range(SIZE):
        for x in range(SIZE):
            if BITS[y][x] == 0:
                canvas[y][x] = BLACK
    return canvas


SCALE = SIZE / 32.0
cur = composite(SCALE, 16.0, 15.0, 7.0)
new = composite(SCALE, 15.4, 15.2, 6.2)


def show(c, label, step=2):
    print(f"\n===== {label} =====")
    for y in range(0, SIZE, step):
        print("".join(c[y][x] for x in range(0, SIZE, step)))


show(cur, "CURRENT  centre(16.0,15.0) r=7.0")
show(new, "PROPOSED centre(15.4,15.2) r=6.2")


def overlaps_ink(canvas):
    """Yellow pixels that sit where the artwork wanted black ink."""
    n = 0
    for y in range(SIZE):
        for x in range(SIZE):
            if BITS[y][x] == 0 and canvas[y][x] == YELLOW:
                n += 1
    return n


print("\n--- quality ---")
print(f"current : yellow={sum(r.count(YELLOW) for r in cur):4}px  "
      f"yellow-painted-over-ink={overlaps_ink(cur):4}px")
print(f"proposed: yellow={sum(r.count(YELLOW) for r in new):4}px  "
      f"yellow-painted-over-ink={overlaps_ink(new):4}px")
