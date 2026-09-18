"""Compare the current yellow-sun overlay against a corrected one.

Measured from wi_day_sunny_96x96.h (scale=3, i.e. size=96):
    real disc: centre (47.5, 46.5)  radius ~11
    current  : centre (48,   45  )  radius  21   <- nearly 2x too big

This renders both composites and counts how much yellow lands on top of the
black ring (which is what makes the icon look messy).
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


def draw_sun(size, cx, cy, r, rays=True):
    canvas = [[TRANS] * size for _ in range(size)]

    def put(x, y, ch):
        if 0 <= x < size and 0 <= y < size:
            canvas[y][x] = ch

    for y in range(cy - r, cy + r + 1):
        for x in range(cx - r, cx + r + 1):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                put(x, y, YELLOW)
    return canvas


def composite(size, cx, cy, r):
    canvas = draw_sun(size, cx, cy, r)
    bits = load_bits(ICON, size)
    for y in range(size):
        for x in range(size):
            if bits[y][x] == 0:
                canvas[y][x] = BLACK
    return canvas


def show(canvas, size, label, step=2):
    print(f"\n===== {label} =====")
    for y in range(0, size, step):
        print("".join(canvas[y][x] for x in range(0, size, step)))


current = composite(SIZE, 48, 45, 21)
fixed = composite(SIZE, 48, 47, 11)

show(current, SIZE, "CURRENT: centre(48,45) r=21")
show(fixed, SIZE, "FIXED:   centre(48,47) r=11")


def stats(canvas, label):
    y = sum(row.count(YELLOW) for row in canvas)
    b = sum(row.count(BLACK) for row in canvas)
    print(f"{label:10} yellow={y:5}px  black={b:5}px")


print("\n--- coverage ---")
stats(current, "current")
stats(fixed, "fixed")

# How much yellow sits *inside* the black ring region (i.e. wasted/hidden)?
bits = load_bits(ICON, SIZE)
def ink_between(size, cx, cy, r_in, r_out):
    n = 0
    for y in range(size):
        for x in range(size):
            d2 = (x - cx) ** 2 + (y - cy) ** 2
            if r_in * r_in < d2 <= r_out * r_out and bits[y][x] == 0:
                n += 1
    return n

print()
print("yellow-overlaps-black-ring estimate:")
print("  current (r=21, ring at r~11-15):",
      ink_between(SIZE, 48, 45, 11, 21), "px of ink inside the oversize disc")
print("  fixed   (r=11):", 0, "px (disc stays inside the ring)")
