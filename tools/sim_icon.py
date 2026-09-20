"""Simulate the composed large weather icon to see why yellow dominates.

Reproduces page_weather.cpp's drawDaytimeWeatherIcon():
  drawYellowSun()                 -> yellow disc + 4 axis-aligned rays
  drawTransparentBlackBitmap()    -> black ink wherever the bitmap bit is 0

Bitmap polarity (from drawTransparentBlackBitmap): bit==0 -> black ink,
bit==1 -> transparent. So '#' in the ASCII dump = transparent, '.' = black.
"""
import re

ICON = r"E:\github\ESP32-Dashboard\src\assets\icons\96x96\wi_day_sunny_96x96.h"

# palette chars for the composite
TRANS = " "   # nothing drawn
YELLOW = "Y"  # highlight colour
BLACK = "#"   # black ink


def load_bits(path, size):
    src = open(path, encoding="utf-8", errors="replace").read()
    body = src[src.find("{"):]
    vals = [int(m, 16) for m in re.findall(r"0[xX]([0-9a-fA-F]{2})", body)]
    bpr = (size + 7) // 8
    grid = []
    for row in range(size):
        line = []
        for col in range(size):
            byte = vals[row * bpr + col // 8]
            line.append((byte >> (7 - (col & 7))) & 1)
        grid.append(line)
    return grid


def draw_yellow_sun(size, partly_cloudy=False):
    """Mirror drawYellowSun(): scale = size/32, centre (16,15)*scale."""
    scale = size // 32
    cx, cy = 16 * scale, 15 * scale
    r = 7 * scale
    ray_off, ray_len = 11 * scale, 4 * scale
    ray_w = max(1, scale)
    canvas = [[TRANS] * size for _ in range(size)]

    def put(x, y, ch):
        if 0 <= x < size and 0 <= y < size:
            canvas[y][x] = ch

    # filled disc
    for y in range(cy - r, cy + r + 1):
        for x in range(cx - r, cx + r + 1):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                put(x, y, YELLOW)
    # 4 cardinal rays
    for i in range(ray_len):
        for w in range(ray_w):
            put(cx - ray_w // 2 + w, cy - ray_off - ray_len + i, YELLOW)
            put(cx - ray_w // 2 + w, cy + ray_off + i, YELLOW)
            put(cx - ray_off - ray_len + i, cy - ray_w // 2 + w, YELLOW)
            put(cx + ray_off + i, cy - ray_w // 2 + w, YELLOW)
    if partly_cloudy:
        def circle(px, py, pr, ch):
            for y in range(py - pr, py + pr + 1):
                for x in range(px - pr, px + pr + 1):
                    if (x - px) ** 2 + (y - py) ** 2 <= pr * pr:
                        put(x, y, ch)
        circle(11 * scale, 19 * scale, 5 * scale, TRANS)
        circle(19 * scale, 18 * scale, 7 * scale, TRANS)
    return canvas


def composite(size):
    canvas = draw_yellow_sun(size)
    bits = load_bits(ICON, size)
    for y in range(size):
        for x in range(size):
            if bits[y][x] == 0:      # black ink
                canvas[y][x] = BLACK
    return canvas


def show(canvas, size, label, step=2):
    print(f"\n===== COMPOSITE: {label} =====")
    for y in range(0, size, step):
        print("".join(canvas[y][x] for x in range(0, size, step)))


if __name__ == "__main__":
    c = composite(96)
    show(c, 96, "wi_day_sunny_96x96 + drawYellowSun (current)")

    # quantify coverage in the active region
    counts = {TRANS: 0, YELLOW: 0, BLACK: 0}
    for row in c:
        for v in row:
            counts[v] += 1
    total = 96 * 96
    print("\ncoverage of the 96x96 canvas:")
    for k, name in ((YELLOW, "yellow"), (BLACK, "black"), (TRANS, "empty")):
        print(f"  {name:7} {counts[k]:5} px  {counts[k]/total*100:5.1f}%")
