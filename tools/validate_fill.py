"""Validate the stack-based flood fill used in drawBitmapWithYellowDisc().

Mirrors the C++ exactly: seed every transparent border pixel, then pop from an
explicit stack and push unvisited transparent neighbours. Checks the tint set
against a BFS ground truth on every size the renderer uses.
"""
import re
import os
from collections import deque

BASE = r"E:\github\ESP32-Dashboard\src\assets\icons"
ICONS = [(96, "wi_day_sunny_96x96.h"),
         (32, "wi_day_sunny_32x32.h")]


def load_bits(path, size):
    src = open(path, encoding="utf-8", errors="replace").read()
    body = src[src.find("{"):]
    vals = [int(m, 16) for m in re.findall(r"0[xX]([0-9a-fA-F]{2})", body)]
    bpr = (size + 7) // 8
    return [[(vals[r * bpr + c // 8] >> (7 - (c & 7))) & 1 for c in range(size)]
            for r in range(size)]


def cpp_stack_fill(bits, W, H):
    visited = [[False] * W for _ in range(H)]
    stack = []
    for c in range(W):
        for r in (0, H - 1):
            if not visited[r][c] and bits[r][c] == 1:
                visited[r][c] = True
                stack.append((r, c))
    for r in range(H):
        for c in (0, W - 1):
            if not visited[r][c] and bits[r][c] == 1:
                visited[r][c] = True
                stack.append((r, c))
    while stack:
        r, c = stack.pop()
        for nr, nc in ((r - 1, c), (r + 1, c), (r, c - 1), (r, c + 1)):
            if 0 <= nr < H and 0 <= nc < W and not visited[nr][nc] and bits[nr][nc] == 1:
                visited[nr][nc] = True
                stack.append((nr, nc))
    return visited


def bfs_reference(bits, W, H):
    seen = [[False] * W for _ in range(H)]
    q = deque()
    for c in range(W):
        for r in (0, H - 1):
            if bits[r][c] == 1 and not seen[r][c]:
                seen[r][c] = True
                q.append((r, c))
    for r in range(H):
        for c in (0, W - 1):
            if bits[r][c] == 1 and not seen[r][c]:
                seen[r][c] = True
                q.append((r, c))
    while q:
        r, c = q.popleft()
        for nr, nc in ((r - 1, c), (r + 1, c), (r, c - 1), (r, c + 1)):
            if 0 <= nr < H and 0 <= nc < W and not seen[nr][nc] and bits[nr][nc] == 1:
                seen[nr][nc] = True
                q.append((nr, nc))
    return seen


all_ok = True
for size, fname in ICONS:
    path = os.path.join(BASE, f"{size}x{size}", fname)
    bits = load_bits(path, size)
    W = H = size
    got = cpp_stack_fill(bits, W, H)
    ref = bfs_reference(bits, W, H)
    diffs = sum(1 for r in range(H) for c in range(W) if got[r][c] != ref[r][c])

    # Pixels that will actually be tinted: transparent AND not reached by flood.
    tint = sum(1 for r in range(H) for c in range(W)
               if bits[r][c] == 1 and not got[r][c])
    ink_total = sum(row.count(0) for row in bits)
    transparent_total = sum(row.count(1) for row in bits)
    bg_visited = sum(1 for r in range(H) for c in range(W)
                     if bits[r][c] == 1 and got[r][c])

    print(f"{size:>3}px  ink={ink_total:5}  transparent={transparent_total:5}  "
          f"tinted={tint:4}  background={bg_visited:5}  diffs_vs_bfs={diffs}")
    if diffs:
        all_ok = False

print()
print("PASS: stack fill matches the BFS ground truth at every size."
      if all_ok else "FAIL: fill disagrees with BFS")
print()
print("Tinted pixels are transparent by construction, so yellow can never")
print("land on top of a black stroke — that was the original visual defect.")
