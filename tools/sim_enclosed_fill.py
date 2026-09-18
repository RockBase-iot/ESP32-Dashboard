"""Colour the bitmap's own disc hole, instead of drawing a second sun.

Current code draws an independent yellow disc+rays via drawYellowSun() and then
stamps the bitmap on top. Two suns drawn from different geometry never line up,
so black ring strokes cut through the yellow. This approach instead finds the
transparent region(s) *enclosed by* the artwork and fills exactly those with
yellow — perfect alignment by construction, since it is the same bitmap.
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


def enclosed_mask(bits, size):
    """Transparent pixels not reachable from the border (flood fill outside)."""
    outside = [[False] * size for _ in range(size)]
    stack = []
    for x in range(size):
        stack.append((0, x))
        stack.append((size - 1, x))
    for y in range(size):
        stack.append((y, 0))
        stack.append((y, size - 1))
    while stack:
        y, x = stack.pop()
        if not (0 <= y < size and 0 <= x < size):
            continue
        if outside[y][x] or bits[y][x] == 0:
            continue
        outside[y][x] = True
        stack.extend([(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)])

    mask = [[False] * size for _ in range(size)]
    for y in range(size):
        for x in range(size):
            if bits[y][x] == 1 and not outside[y][x]:
                mask[y][x] = True
    return mask


BITS = load_bits(ICON, SIZE)
MASK = enclosed_mask(BITS, SIZE)

canvas = [[TRANS] * SIZE for _ in range(SIZE)]
for y in range(SIZE):
    for x in range(SIZE):
        if BITS[y][x] == 0:
            canvas[y][x] = BLACK
        elif MASK[y][x]:
            canvas[y][x] = YELLOW


def show(c, label, step=2):
    print(f"\n===== {label} =====")
    for y in range(0, SIZE, step):
        print("".join(c[y][x] for x in range(0, SIZE, step)))


show(canvas, "ENCLOSED-FILL: bitmap disc hole tinted yellow")

ys = sum(r.count(YELLOW) for r in canvas)
bs = sum(r.count(BLACK) for r in canvas)
print(f"\nyellow={ys}px  black={bs}px")
print("yellow never covers black ink: by construction (mask excludes ink)")
