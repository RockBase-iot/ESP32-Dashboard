"""Render the weather-icon bitmaps as ASCII art so their shape is visible.

The icon headers store 1-bit-per-pixel images in XBM-like byte arrays
(MSB first, rows padded to whole bytes). This prints them so we can judge
where the yellow highlight lands relative to the drawn artwork.
"""
import re
import sys

ICON = r"E:\github\ESP32-Dashboard\src\assets\icons\96x96\wi_day_sunny_96x96.h"
ICON2 = r"E:\github\ESP32-Dashboard\src\assets\icons\96x96\wi_day_sunny_overcast_96x96.h"


def load_bytes(path):
    src = open(path, encoding="utf-8", errors="replace").read()
    # Grab every 0x?? literal in the array body
    body_start = src.find("{")
    body = src[body_start:]
    vals = [int(m, 16) for m in re.findall(r"0[xX]([0-9a-fA-F]{2})", body)]
    return vals


def render(path, size, label):
    vals = load_bytes(path)
    need = size * ((size + 7) // 8)
    if len(vals) < need:
        print(f"{label}: only {len(vals)} bytes, need {need}")
        return
    bpr = (size + 7) // 8
    print(f"\n===== {label}  ({size}x{size}, {bpr} bytes/row) =====")
    for row in range(size):
        line = []
        for col in range(size):
            byte = vals[row * bpr + col // 8]
            bit = (byte >> (7 - (col & 7))) & 1
            line.append("#" if bit else ".")
        print("".join(line))


if __name__ == "__main__":
    render(ICON, 96, "wi_day_sunny_96x96")
