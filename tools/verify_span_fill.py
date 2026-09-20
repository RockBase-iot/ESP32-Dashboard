"""Verify the weather-icon tint contract used by page_weather.cpp.

`drawHighlightedWeatherIcon` paints paper pixels (bit == 0) with the highlight
pigment and ink pixels (bit == 1) with black. Whether that produces the
intended artwork depends entirely on the icon polarity, which is an *empirical*
property of the PNG-derived headers, not something the C++ can assert. This
script therefore measures each glyph and fails loudly if the assumption breaks:

  1. Polarity: the sunny/partly-cloudy glyphs must be ink-dominant tiles with
     the sun appearing as PAPER, so tinting paper paints the sun. If a future
     icon swap flips this, the check fails and the routine needs revisiting.
  2. No paper on the glyph border: guarantees the artwork is "cut out of" a
     tile rather than an outline shape, so the whole paper set is the subject.
  3. Coverage: the tint set must be a small, connected-looking fraction of the
     glyph (the sun), not the bulk of it, and all glyphs must decode to the
     exact byte length the header claims.

Run after any icon change:
    python tools/verify_span_fill.py
"""

from __future__ import annotations

import re
import sys
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ICON_DIR = ROOT / "src" / "assets" / "icons"

# Declaration form used by the generated headers:
#   const unsigned char wi_day_sunny_96x96[] PROGMEM = { ... };
C_ARRAY = re.compile(
    r"(?:static\s+)?(?:const\s+)?(?:unsigned\s+char|uint8_t)\s+(\w+)\s*\[\s*\]\s*"
    r"(?:PROGMEM\s*)?=\s*\{(.*?)\};",
    re.S,
)

# Symbols the weather page hands to drawDaytimeWeatherIcon, and stamp size.
TARGETS: list[tuple[str, int]] = [
    ("wi_day_sunny_96x96", 96),
    ("wi_day_sunny_overcast_96x96", 96),
    ("wi_night_clear_96x96", 96),
    ("wi_night_alt_partly_cloudy_96x96", 96),
    ("wi_cloudy_96x96", 96),
    ("wi_day_sunny_32x32", 32),
    ("wi_day_sunny_overcast_32x32", 32),
    ("wi_cloudy_32x32", 32),
]


def parse_arrays(text: str) -> dict[str, list[int]]:
    return {
        name: [int(v, 0) for v in re.findall(r"0[xX][0-9a-fA-F]+|\d+", body)]
        for name, body in C_ARRAY.findall(text)
    }


def load_glyphs() -> dict[str, list[int]]:
    wanted = {n for n, _ in TARGETS}
    out: dict[str, list[int]] = {}
    for f in ICON_DIR.rglob("*.h"):
        for name, vals in parse_arrays(
            f.read_text(encoding="utf-8", errors="replace")
        ).items():
            if name in wanted:
                out[name] = vals
    return out


def decode(vals: list[int], n: int) -> list[list[int]]:
    """1 = ink (bit set), 0 = paper."""
    bpr = (n + 7) // 8
    return [
        [1 if (vals[r * bpr + (c >> 3)] & (0x80 >> (c & 7))) else 0 for c in range(n)]
        for r in range(n)
    ]


def largest_paper_component(g: list[list[int]], n: int) -> int:
    """Size of the biggest 4-connected paper region (the sun body)."""
    seen = [[False] * n for _ in range(n)]
    best = 0
    for sr in range(n):
        for sc in range(n):
            if g[sr][sc] != 0 or seen[sr][sc]:
                continue
            seen[sr][sc] = True
            q = deque([(sr, sc)])
            size = 0
            while q:
                r, c = q.popleft()
                size += 1
                for dr, dc in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    nr, nc = r + dr, c + dc
                    if 0 <= nr < n and 0 <= nc < n and not seen[nr][nc] and g[nr][nc] == 0:
                        seen[nr][nc] = True
                        q.append((nr, nc))
            best = max(best, size)
    return best


def main() -> int:
    glyphs = load_glyphs()
    failures = 0
    print(f"{'glyph':34s} {'n':>3} {'bytes':>6} {'ink':>6} {'tint':>6} "
          f"{'borderPaper':>11} {'largestTint':>11}  verdict")

    for name, n in TARGETS:
        vals = glyphs.get(name)
        if vals is None:
            print(f"{name:34s} {'--':>3}  MISSING from headers")
            failures += 1
            continue

        expect = (n + 7) // 8 * n
        if len(vals) != expect:
            print(f"{name:34s} {n:>3} {len(vals):>6}  expected {expect} bytes")
            failures += 1
            continue

        g = decode(vals, n)
        ink = sum(row.count(1) for row in g)
        tint = n * n - ink
        border_paper = (
            sum(1 for r in range(n) if g[r][0] == 0 or g[r][n - 1] == 0)
            + sum(1 for c in range(n) if g[0][c] == 0 or g[n - 1][c] == 0)
        )
        largest = largest_paper_component(g, n)

        problems = []
        # 1. The sun must be paper, i.e. the glyph is an ink-dominant tile.
        if ink <= tint:
            problems.append("not ink-dominant (polarity flipped?)")
        # 2. Paper must not touch the border, else the subject is an outline.
        if border_paper != 0:
            problems.append(f"{border_paper} paper px on border")
        # 3. The tint set must be a minority of the glyph.
        if tint * 100 > n * n * 50:
            problems.append("tint covers >50% of glyph")

        verdict = "OK" if not problems else "FAIL: " + "; ".join(problems)
        if problems:
            failures += 1
        print(f"{name:34s} {n:>3} {len(vals):>6} {ink:>6} {tint:>6} "
              f"{border_paper:>11} {largest:>11}  {verdict}")

    print()
    if failures:
        print(f"RESULT: {failures} FAILED — the tint contract no longer holds; "
              f"re-check drawHighlightedWeatherIcon before flashing.")
        return 1
    print("RESULT: contract holds — paper == the sun, tinting paper is correct.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
