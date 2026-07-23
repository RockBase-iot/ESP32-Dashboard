from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable

from PIL import Image, ImageDraw, ImageFont


BASE_W = 400
BASE_H = 300
SCALE = 4
W = BASE_W * SCALE
H = BASE_H * SCALE

BG = (248, 247, 242)
BLACK = (16, 18, 20)
RED = (178, 30, 45)
GRAY = (128, 128, 128)
LIGHT = (232, 231, 226)
WHITE = (255, 255, 255)

TIME_LABEL = "WED 14:32 JUL 22, 2026"
IP_LABEL = "IP: 192.168.1.42"
BATTERY_LABEL = "82%"


def p(value: float) -> int:
    return int(round(value * SCALE))


def font_path(preferred: Iterable[str]) -> str | None:
    for candidate in preferred:
        path = Path(candidate)
        if path.exists():
            return str(path)
    return None


FONT_REGULAR = font_path(
    [
        r"C:\Windows\Fonts\segoeui.ttf",
        r"C:\Windows\Fonts\arial.ttf",
    ]
)
FONT_BOLD = font_path(
    [
        r"C:\Windows\Fonts\segoeuib.ttf",
        r"C:\Windows\Fonts\arialbd.ttf",
    ]
)
FONT_MONO = font_path(
    [
        r"C:\Windows\Fonts\consola.ttf",
        r"C:\Windows\Fonts\cour.ttf",
    ]
)


def make_font(size: int, bold: bool = False, mono: bool = False) -> ImageFont.FreeTypeFont:
    selected = FONT_MONO if mono else (FONT_BOLD if bold else FONT_REGULAR)
    if selected:
        return ImageFont.truetype(selected, p(size))
    return ImageFont.load_default()


def text_width(draw: ImageDraw.ImageDraw, text: str, size: int, bold: bool = False, mono: bool = False) -> int:
    bbox = draw.textbbox((0, 0), text, font=make_font(size, bold=bold, mono=mono))
    return bbox[2] - bbox[0]


def draw_text(
    draw: ImageDraw.ImageDraw,
    xy: tuple[float, float],
    text: str,
    size: int,
    fill: tuple[int, int, int] = BLACK,
    bold: bool = False,
    mono: bool = False,
    anchor: str = "la",
) -> None:
    draw.text((p(xy[0]), p(xy[1])), text, font=make_font(size, bold=bold, mono=mono), fill=fill, anchor=anchor)


def fit_text(draw: ImageDraw.ImageDraw, text: str, size: int, max_w: float, bold: bool = False, mono: bool = False) -> str:
    limit = p(max_w)
    if text_width(draw, text, size, bold=bold, mono=mono) <= limit:
        return text
    suffix = ".."
    out = suffix
    for idx in range(1, len(text) + 1):
        candidate = text[:idx] + suffix
        if text_width(draw, candidate, size, bold=bold, mono=mono) > limit:
            break
        out = candidate
    return out


def rect(draw: ImageDraw.ImageDraw, box: tuple[float, float, float, float], outline=BLACK, width: int = 1, fill=None) -> None:
    scaled = tuple(p(v) for v in box)
    draw.rectangle(scaled, outline=outline, width=max(1, p(width)), fill=fill)


def line(draw: ImageDraw.ImageDraw, points: Iterable[tuple[float, float]], fill=BLACK, width: int = 1) -> None:
    draw.line([(p(x), p(y)) for x, y in points], fill=fill, width=max(1, p(width)))


def rounded(draw: ImageDraw.ImageDraw, box: tuple[float, float, float, float], radius: int = 0, outline=BLACK, width: int = 1, fill=None) -> None:
    scaled = tuple(p(v) for v in box)
    draw.rounded_rectangle(scaled, radius=p(radius), outline=outline, width=max(1, p(width)), fill=fill)


def draw_wifi_icon(draw: ImageDraw.ImageDraw, x: float, y: float, color=BLACK) -> None:
    cx = x + 10
    cy = y + 17
    for radius in (17, 11, 5):
        draw.arc(
            (p(cx - radius), p(cy - radius), p(cx + radius), p(cy + radius)),
            220,
            320,
            fill=color,
            width=p(1.7),
        )
    draw.ellipse((p(cx - 2), p(cy + 1), p(cx + 2), p(cy + 5)), fill=color)


def draw_battery_icon(draw: ImageDraw.ImageDraw, x: float, y: float, color=BLACK) -> None:
    rect(draw, (x, y + 1, x + 15, y + 9), outline=color, width=1)
    rect(draw, (x + 15, y + 4, x + 17, y + 6), outline=color, fill=color)
    rect(draw, (x + 2, y + 3, x + 12, y + 7), outline=RED, fill=RED)


def draw_page_icon(draw: ImageDraw.ImageDraw, icon: str, x: float, y: float) -> None:
    if icon == "overview":
        rect(draw, (x, y, x + 14, y + 14), width=1)
        line(draw, [(x + 3, y + 5), (x + 11, y + 5)], width=1)
        line(draw, [(x + 3, y + 9), (x + 9, y + 9)], fill=RED, width=1)
    elif icon == "month":
        rect(draw, (x, y + 1, x + 15, y + 14), width=1)
        line(draw, [(x, y + 5), (x + 15, y + 5)], width=1)
        rect(draw, (x + 8, y + 8, x + 11, y + 11), outline=RED, fill=RED)
    elif icon == "week":
        line(draw, [(x + 2, y + 13), (x + 2, y + 2), (x + 13, y + 2)], width=1)
        line(draw, [(x + 4, y + 11), (x + 7, y + 8), (x + 10, y + 9), (x + 13, y + 4)], fill=RED, width=2)
    elif icon == "clock":
        draw.ellipse((p(x), p(y), p(x + 15), p(y + 15)), outline=BLACK, width=p(1))
        line(draw, [(x + 7.5, y + 7.5), (x + 7.5, y + 3)], width=1)
        line(draw, [(x + 7.5, y + 7.5), (x + 11, y + 9)], fill=RED, width=1)
    elif icon == "news":
        rect(draw, (x, y + 2, x + 15, y + 14), width=1)
        line(draw, [(x + 3, y + 5), (x + 12, y + 5)], fill=RED, width=1)
        line(draw, [(x + 3, y + 9), (x + 12, y + 9)], width=1)
    elif icon == "weather":
        draw.ellipse((p(x + 1), p(y + 5), p(x + 8), p(y + 12)), outline=RED, width=p(1))
        draw.arc((p(x + 6), p(y + 3), p(x + 17), p(y + 14)), 180, 360, fill=BLACK, width=p(1))
        line(draw, [(x + 5, y + 12), (x + 16, y + 12)], width=1)
    elif icon == "portfolio":
        rect(draw, (x + 1, y + 4, x + 15, y + 14), width=1)
        line(draw, [(x + 5, y + 4), (x + 5, y + 1), (x + 11, y + 1), (x + 11, y + 4)], width=1)
        line(draw, [(x + 4, y + 10), (x + 7, y + 7), (x + 10, y + 9), (x + 13, y + 5)], fill=RED, width=1)
    elif icon == "economic":
        rect(draw, (x, y + 2, x + 15, y + 14), width=1)
        line(draw, [(x + 4, y + 11), (x + 4, y + 7)], fill=RED, width=2)
        line(draw, [(x + 8, y + 11), (x + 8, y + 5)], width=2)
        line(draw, [(x + 12, y + 11), (x + 12, y + 3)], width=2)


def draw_header(draw: ImageDraw.ImageDraw, title: str, icon: str) -> None:
    draw_page_icon(draw, icon, 18, 13)
    draw_text(draw, (42, 18), title, 14, bold=True, anchor="lm")
    draw_wifi_icon(draw, 282, 11)
    line(draw, [(306, 12), (306, 25)], fill=GRAY, width=1)
    draw_battery_icon(draw, 318, 11)
    draw_text(draw, (382, 18), BATTERY_LABEL, 10, anchor="rm")
    draw_text(draw, (382, 34), TIME_LABEL, 8, fill=BLACK, anchor="rm")
    line(draw, [(18, 48), (382, 48)], width=1)
    line(draw, [(18, 50), (92, 50)], fill=RED, width=2)


def draw_footer(draw: ImageDraw.ImageDraw, page_no: int, total_pages: int) -> None:
    draw_text(draw, (18, 286), IP_LABEL, 8, fill=GRAY, anchor="lm")
    draw_text(draw, (382, 286), f"{page_no} | {total_pages}", 8, fill=GRAY, anchor="rm")


def draw_shell(title: str, icon: str, page_no: int, total_pages: int) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    image = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(image)
    draw_header(draw, title, icon)
    draw_footer(draw, page_no, total_pages)
    return image, draw


def panel(draw: ImageDraw.ImageDraw, box: tuple[float, float, float, float], title: str | None = None) -> None:
    rounded(draw, box, radius=0, outline=BLACK, width=1, fill=BG)
    if title:
        draw_text(draw, (box[0] + 8, box[1] + 15), title, 8, fill=RED, bold=True, anchor="lm")
        line(draw, [(box[0] + 6, box[1] + 24), (box[2] - 6, box[1] + 24)], width=1)


def draw_overview(draw: ImageDraw.ImageDraw) -> None:
    for i, day in enumerate(["MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"]):
        x0 = 32 + i * 54
        color = RED if day == "WED" else BLACK
        draw_text(draw, (x0, 68), day, 7, fill=color if day == "WED" else GRAY, bold=True, anchor="mm")
        draw_text(draw, (x0, 82), str(20 + i), 10, fill=color, bold=True, anchor="mm")
    line(draw, [(18, 93), (382, 93)], width=1)

    panel(draw, (18, 106, 248, 246), "TODAY AGENDA")
    rows = [
        ("09:00", "User review", "Project Atlas"),
        ("11:30", "Design sync", "Layout freeze"),
        ("15:00", "Firmware flash", "NM-EPD-420"),
    ]
    for idx, (time, name, detail) in enumerate(rows):
        y = 151 + idx * 34
        rect(draw, (28, y - 15, 238, y + 13), outline=LIGHT, fill=WHITE)
        draw_text(draw, (36, y), time, 9, fill=RED, bold=True, mono=True, anchor="lm")
        draw_text(draw, (91, y - 4), name, 9, bold=True, anchor="lm")
        draw_text(draw, (91, y + 8), detail, 7, fill=GRAY, anchor="lm")

    panel(draw, (262, 106, 382, 174), "NOTES")
    for idx, item in enumerate(["Buy milk", "Call supplier"]):
        y = 148 + idx * 18
        rect(draw, (274, y - 5, 280, y + 1), outline=RED, fill=RED)
        draw_text(draw, (288, y), item, 8, anchor="lm")

    panel(draw, (262, 186, 382, 246), "MILESTONES")
    draw_text(draw, (274, 226), "Task 14 visual", 8, bold=True, anchor="lm")
    draw_text(draw, (274, 240), "Prototype refresh", 7, fill=GRAY, anchor="lm")


def draw_monthly(draw: ImageDraw.ImageDraw) -> None:
    draw_text(draw, (20, 68), "JULY 2026", 14, bold=True, anchor="lm")
    draw_text(draw, (382, 68), "12 EVENTS", 8, fill=GRAY, anchor="rm")
    for i, day in enumerate(["MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"]):
        x = 32 + i * 54
        color = RED if day == "WED" else GRAY
        draw_text(draw, (x, 91), day, 7, fill=color, bold=True, anchor="mm")
        draw_text(draw, (x, 105), str(20 + i), 10, fill=RED if day == "WED" else BLACK, bold=True, anchor="mm")
    line(draw, [(18, 116), (382, 116)], width=1)

    events = [
        ("04", "SAT", "09:00", "Holiday", "Family calendar"),
        ("08", "WED", "10:30", "Dentist", "Downtown clinic"),
        ("10", "FRI", "14:00", "Q3 Review", "Finance deck"),
        ("17", "FRI", "17:30", "Deadline", "Prototype freeze"),
        ("20", "MON", "09:00", "Review", "Task 14 UI"),
        ("22", "WED", "18:30", "Birthday", "Dinner reservation"),
        ("25", "SAT", "16:00", "Soccer", "Community field"),
        ("28", "TUE", "11:00", "Atlas", "Project sync"),
    ]
    for idx, (date, weekday, time, title, detail) in enumerate(events):
        col = idx % 2
        row = idx // 2
        x0 = 18 + col * 188
        y0 = 128 + row * 34
        active = date == "22"
        rect(draw, (x0, y0, x0 + 176, y0 + 26), outline=RED if active else LIGHT, fill=WHITE)
        draw_text(draw, (x0 + 12, y0 + 13), date, 13, fill=RED if active else BLACK, bold=True, mono=True, anchor="lm")
        draw_text(draw, (x0 + 44, y0 + 8), weekday, 7, fill=RED if active else GRAY, bold=True, anchor="lm")
        draw_text(draw, (x0 + 44, y0 + 20), time, 7, fill=RED if active else BLACK, mono=True, anchor="lm")
        draw_text(draw, (x0 + 93, y0 + 9), fit_text(draw, title, 8, 72, bold=True), 8, bold=True, anchor="lm")
        draw_text(draw, (x0 + 93, y0 + 21), fit_text(draw, detail, 7, 72), 7, fill=GRAY, anchor="lm")


def draw_weekly(draw: ImageDraw.ImageDraw) -> None:
    draw_text(draw, (20, 68), "JUL 20-26 2026", 12, bold=True, anchor="lm")
    week = [
        ("MON", 20, [("10:00", "Focus")]),
        ("TUE", 21, [("10:00", "Focus")]),
        ("WED", 22, [("09:00", "Review"), ("13:30", "Sync")]),
        ("THU", 23, [("10:00", "Focus")]),
        ("FRI", 24, [("10:00", "Focus")]),
        ("SAT", 25, [("16:00", "Soccer")]),
        ("SUN", 26, [("11:00", "Family")]),
    ]
    for idx, (day, date, events) in enumerate(week):
        x0 = 18 + idx * 52
        active = day == "WED"
        panel(draw, (x0, 84, x0 + 46, 246))
        draw_text(draw, (x0 + 6, 102), day, 7, fill=RED if active else BLACK, bold=True, anchor="lm")
        draw_text(draw, (x0 + 40, 102), str(date), 8, fill=RED if active else GRAY, bold=True, anchor="rm")
        for eidx, (time, title) in enumerate(events):
            y = 132 + eidx * 44
            rect(draw, (x0 + 5, y - 12, x0 + 41, y + 22), outline=RED if active and eidx == 0 else LIGHT, fill=WHITE)
            draw_text(draw, (x0 + 9, y), time, 6, fill=RED if active and eidx == 0 else BLACK, mono=True, anchor="lm")
            draw_text(draw, (x0 + 9, y + 13), fit_text(draw, title, 6, 30), 6, anchor="lm")


def draw_world_clock(draw: ImageDraw.ImageDraw) -> None:
    cards = [(18, 64, 194, 134), (206, 64, 382, 134), (18, 150, 194, 220), (206, 150, 382, 220)]
    rows = [
        ("NEW YORK", "01:32", "EDT - WORKING", False),
        ("LONDON", "06:32", "BST - WORKING", True),
        ("BERLIN", "07:32", "CEST - WORKING", False),
        ("SHANGHAI", "14:32", "CST - EVENING", False),
    ]
    for box, (city, time, status, accent) in zip(cards, rows):
        panel(draw, box)
        city_y = box[1] + 15
        status_y = box[3] - 12
        time_y = (city_y + status_y) / 2
        draw_text(draw, (box[0] + 8, city_y), city, 8, anchor="lm")
        draw_text(draw, (box[0] + 8, time_y), time, 21, fill=RED if accent else BLACK, bold=True, mono=True, anchor="lm")
        draw_text(draw, (box[0] + 8, status_y), status, 8, fill=RED if accent else GRAY, anchor="lm")


def draw_news(draw: ImageDraw.ImageDraw) -> None:
    panel(draw, (18, 62, 382, 132), "TOP STORIES")
    draw_text(draw, (30, 105), "E-paper dashboard reaches Task 14 review", 11, bold=True, anchor="lm")
    draw_text(draw, (30, 122), "Local cache and direct feeds remain read-only", 8, fill=GRAY, anchor="lm")
    for idx, title in enumerate(["Weather service latency improves", "Markets wait for CPI print", "Calendar sync window trimmed"]):
        y0 = 144 + idx * 31
        rect(draw, (18, y0, 382, y0 + 26), outline=LIGHT, fill=WHITE)
        rect(draw, (28, y0 + 8, 34, y0 + 18), outline=RED, fill=RED)
        draw_text(draw, (44, y0 + 14), title, 9, anchor="lm")
    panel(draw, (18, 238, 382, 266), None)
    draw_text(draw, (30, 252), "1969", 12, fill=RED, bold=True, anchor="lm")
    draw_text(draw, (86, 252), "Apollo 11 completes the first moonwalk", 8, anchor="lm")


def draw_weather(draw: ImageDraw.ImageDraw) -> None:
    panel(draw, (18, 62, 128, 132))
    draw_text(draw, (30, 81), "NEW YORK", 8, bold=True, anchor="lm")
    draw_text(draw, (30, 106), "72F", 22, fill=RED, bold=True, anchor="lm")
    draw_text(draw, (30, 124), "Sunny", 7, anchor="lm")
    panel(draw, (140, 62, 382, 220), None)
    draw_text(draw, (152, 78), "HOURLY", 8, fill=RED, bold=True, anchor="lm")
    chart_left, chart_top, chart_right, chart_bottom = 162, 96, 350, 210
    temp_ticks = [80, 75, 70, 65, 60, 55]
    precip_ticks = [100, 80, 60, 40, 20, 0]
    temp_top, temp_bottom = 96, 202
    bar_bottom, bar_top = 202, 118
    tick_gap = (temp_bottom - temp_top) / (len(temp_ticks) - 1)
    for idx, tick in enumerate(temp_ticks):
        y = temp_top + idx * tick_gap
        line(draw, [(chart_left, y), (chart_right, y)], fill=LIGHT if idx else GRAY, width=1)
        draw_text(draw, (194, y + 1), f"{tick}°", 7, fill=BLACK if idx != len(temp_ticks) - 1 else GRAY, anchor="rm")
    for idx, tick in enumerate(precip_ticks):
        y = temp_top + idx * tick_gap
        draw_text(draw, (358, y + 1), f"{tick}%", 7, fill=BLACK if idx != len(precip_ticks) - 1 else GRAY, anchor="lm")
    hours = ["00", "03", "06", "09", "12", "15", "18", "21"]
    temps = [68, 65, 61, 64, 70, 71, 71, 70]
    precip = [8, 32, 70, 38, 0, 0, 0, 12]
    xs = [chart_left + 4 + i * ((chart_right - chart_left - 8) / (len(hours) - 1)) for i in range(len(hours))]
    def temp_y(val: float) -> float:
        return temp_bottom - (val - 55) * (temp_bottom - temp_top) / (80 - 55)
    def precip_y(val: float) -> float:
        return bar_bottom - val * (bar_bottom - bar_top) / 100
    for idx in range(len(xs) - 1):
        line(draw, [(xs[idx], temp_y(temps[idx])), (xs[idx + 1], temp_y(temps[idx + 1]))], fill=(96, 36, 48), width=2)
    for idx, val in enumerate(precip):
        if val <= 0:
            continue
        x = xs[idx] - 4
        y = precip_y(val)
        rect(draw, (x, y, x + 8, bar_bottom), outline=GRAY, fill=(164, 164, 168))
    for idx, hour in enumerate(hours):
        x = xs[idx]
        draw_text(draw, (x, 206), hour, 7, fill=BLACK if hour == "00" else GRAY, anchor="mm")
    rounded(draw, (18, 230, 382, 274), radius=0, outline=BLACK, width=1, fill=BG)
    draw_text(draw, (28, 244), "WEEKLY WEATHER", 7, fill=RED, bold=True, anchor="lm")
    line(draw, [(28, 250), (372, 250)], width=1)
    week = [
        ("WED", 72, 58),
        ("THU", 74, 60),
        ("FRI", 70, 57),
        ("SAT", 68, 55),
        ("SUN", 71, 56),
        ("MON", 73, 59),
        ("TUE", 75, 61),
    ]
    for idx, (day, hi, lo) in enumerate(week):
        x0 = 38 + idx * 54
        draw_text(draw, (x0, 205), day, 7, fill=RED if idx == 0 else BLACK, bold=True, anchor="mm")
        draw_text(draw, (x0, 225), str(hi), 13, bold=True, anchor="mm")
        draw_text(draw, (x0, 240), str(lo), 7, fill=GRAY, anchor="mm")


def draw_weather_expanded(draw: ImageDraw.ImageDraw) -> None:
    panel(draw, (18, 58, 136, 230), None)
    draw_text(draw, (30, 78), "NEW YORK", 9, bold=True, anchor="lm")
    draw_text(draw, (30, 96), "NY", 8, fill=GRAY, bold=True, anchor="lm")
    draw_text(draw, (30, 113), "USA", 8, fill=GRAY, bold=True, anchor="lm")
    line(draw, [(30, 125), (124, 125)], fill=LIGHT, width=1)
    draw_text(draw, (30, 164), "72F", 24, fill=RED, bold=True, anchor="lm")
    draw_text(draw, (30, 205), "Sunny", 9, anchor="lm")

    panel(draw, (148, 58, 382, 230), None)
    draw_text(draw, (152, 73), "HOURLY", 8, fill=RED, bold=True, anchor="lm")
    chart_left, chart_right = 168, 348
    temp_top, temp_bottom = 88, 210
    bar_bottom, bar_top = 210, 112
    temp_ticks = [80, 75, 70, 65, 60, 55]
    precip_ticks = [100, 80, 60, 40, 20, 0]
    tick_gap = (temp_bottom - temp_top) / (len(temp_ticks) - 1)

    for idx, tick in enumerate(temp_ticks):
        y = temp_top + idx * tick_gap
        line(draw, [(chart_left, y), (chart_right, y)], fill=LIGHT if idx else GRAY, width=1)
        draw_text(draw, (158, y + 1), f"{tick}{chr(176)}", 7, fill=BLACK if idx != len(temp_ticks) - 1 else GRAY, anchor="rm")
    for idx, tick in enumerate(precip_ticks):
        y = temp_top + idx * tick_gap
        draw_text(draw, (356, y + 1), f"{tick}%", 7, fill=BLACK if idx != len(precip_ticks) - 1 else GRAY, anchor="lm")

    hours = ["00", "03", "06", "09", "12", "15", "18", "21"]
    temps = [68, 65, 61, 64, 70, 71, 71, 70]
    precip = [8, 32, 70, 38, 0, 0, 0, 12]
    xs = [chart_left + 4 + i * ((chart_right - chart_left - 8) / (len(hours) - 1)) for i in range(len(hours))]

    def temp_y(val: float) -> float:
        return temp_bottom - (val - 55) * (temp_bottom - temp_top) / (80 - 55)

    def precip_y(val: float) -> float:
        return bar_bottom - val * (bar_bottom - bar_top) / 100

    for idx in range(len(xs) - 1):
        line(draw, [(xs[idx], temp_y(temps[idx])), (xs[idx + 1], temp_y(temps[idx + 1]))], fill=(96, 36, 48), width=2)
    for idx, val in enumerate(precip):
        if val <= 0:
            continue
        x = xs[idx] - 5
        y = precip_y(val)
        rect(draw, (x, y, x + 10, bar_bottom), outline=GRAY, fill=(164, 164, 168))
    for idx, hour in enumerate(hours):
        draw_text(draw, (xs[idx], 218), hour, 7, fill=BLACK if hour == "00" else GRAY, anchor="mm")

    rounded(draw, (18, 238, 382, 278), radius=0, outline=BLACK, width=1, fill=BG)
    draw_text(draw, (28, 250), "WEEKLY WEATHER", 7, fill=RED, bold=True, anchor="lm")
    line(draw, [(28, 256), (372, 256)], width=1)
    week = [
        ("WED", 72, 58),
        ("THU", 74, 60),
        ("FRI", 70, 57),
        ("SAT", 68, 55),
        ("SUN", 71, 56),
        ("MON", 73, 59),
        ("TUE", 75, 61),
    ]
    for idx, (day, hi, lo) in enumerate(week):
        x0 = 42 + idx * 50
        draw_text(draw, (x0, 264), day, 5, fill=RED if idx == 0 else BLACK, bold=True, anchor="mm")
        draw_text(draw, (x0, 272), f"{hi}/{lo}", 5, fill=BLACK if idx != 0 else RED, bold=idx == 0, anchor="mm")


def draw_daily_weather_icon(draw: ImageDraw.ImageDraw, x: float, y: float, code: str, color=BLACK) -> None:
    if code == "sun":
        draw.ellipse((p(x + 5), p(y + 5), p(x + 17), p(y + 17)), outline=color, width=p(1.2))
        for x0, y0, x1, y1 in [
            (11, 0, 11, 4),
            (11, 18, 11, 22),
            (0, 11, 4, 11),
            (18, 11, 22, 11),
            (3, 3, 6, 6),
            (16, 16, 19, 19),
            (16, 6, 19, 3),
            (3, 19, 6, 16),
        ]:
            line(draw, [(x + x0, y + y0), (x + x1, y + y1)], fill=color, width=1)
    elif code == "rain":
        draw.arc((p(x + 1), p(y + 4), p(x + 13), p(y + 17)), 180, 360, fill=color, width=p(1.2))
        draw.arc((p(x + 8), p(y + 1), p(x + 22), p(y + 18)), 180, 360, fill=color, width=p(1.2))
        line(draw, [(x + 3, y + 16), (x + 22, y + 16)], fill=color, width=1)
        for dx in (6, 12, 18):
            line(draw, [(x + dx, y + 20), (x + dx - 2, y + 25)], fill=GRAY, width=1)
    else:
        draw.arc((p(x + 1), p(y + 7), p(x + 14), p(y + 20)), 180, 360, fill=color, width=p(1.2))
        draw.arc((p(x + 9), p(y + 3), p(x + 24), p(y + 21)), 180, 360, fill=color, width=p(1.2))
        line(draw, [(x + 4, y + 19), (x + 24, y + 19)], fill=color, width=1)


def draw_weekly_weather(draw: ImageDraw.ImageDraw) -> None:
    days = [
        ("YESTERDAY", "TUE", "JUL 21", "cloud", 70, 59),
        ("TODAY", "WED", "JUL 22", "rain", 68, 57),
        ("THU", "THU", "JUL 23", "sun", 74, 60),
        ("FRI", "FRI", "JUL 24", "sun", 76, 62),
        ("SAT", "SAT", "JUL 25", "cloud", 71, 58),
    ]

    panel(draw, (18, 62, 382, 166), None)
    draw_text(draw, (30, 80), "5-DAY TREND", 8, fill=RED, bold=True, anchor="lm")
    draw_text(draw, (370, 80), "HIGH / LOW", 7, fill=GRAY, bold=True, anchor="rm")

    chart_left, chart_right = 54, 348
    chart_top, chart_bottom = 88, 145
    for tick in (80, 70, 60, 50):
        y = chart_bottom - (tick - 50) * (chart_bottom - chart_top) / 30
        line(draw, [(chart_left, y), (chart_right, y)], fill=LIGHT if tick != 70 else GRAY, width=1)
        draw_text(draw, (42, y), str(tick), 6, fill=GRAY, anchor="rm")

    xs = [chart_left + i * ((chart_right - chart_left) / 4) for i in range(5)]

    def temp_y(value: int) -> float:
        return chart_bottom - (value - 50) * (chart_bottom - chart_top) / 30

    high_points = [(xs[idx], temp_y(day[4])) for idx, day in enumerate(days)]
    low_points = [(xs[idx], temp_y(day[5])) for idx, day in enumerate(days)]
    line(draw, high_points, fill=RED, width=2)
    line(draw, low_points, fill=BLACK, width=1)
    for idx, day in enumerate(days):
        active = day[1] == "WED"
        x = xs[idx]
        draw.ellipse((p(x - 2.5), p(temp_y(day[4]) - 2.5), p(x + 2.5), p(temp_y(day[4]) + 2.5)), fill=RED)
        rect(draw, (x - 2, temp_y(day[5]) - 2, x + 2, temp_y(day[5]) + 2), outline=BLACK, fill=BLACK)
        draw_text(draw, (x, 156), day[1], 6, fill=RED if active else GRAY, bold=True, anchor="mm")

    card_top, card_bottom = 178, 246
    card_gap = 4
    card_w = (364 - card_gap * 4) / 5
    for idx, day in enumerate(days):
        x0 = 18 + idx * (card_w + card_gap)
        active = day[1] == "WED"
        rect(draw, (x0, card_top, x0 + card_w, card_bottom), outline=RED if active else BLACK, fill=WHITE)
        draw_text(draw, (x0 + card_w / 2, card_top + 12), day[0], 5, fill=RED if active else GRAY, bold=True, anchor="mm")
        draw_text(draw, (x0 + card_w / 2, card_top + 23), day[2], 6, fill=BLACK, bold=True, anchor="mm")
        draw_daily_weather_icon(draw, x0 + card_w / 2 - 12, card_top + 31, day[3], RED if active else BLACK)
        draw_text(draw, (x0 + card_w / 2, card_bottom - 8), f"{day[4]}/{day[5]}", 8,
                  fill=RED if active else BLACK, bold=active, mono=True, anchor="mm")

    draw_text(draw, (28, 264), "Rain eases after today. Warmer highs return into Friday.", 7,
              fill=GRAY, anchor="lm")


def draw_portfolio(draw: ImageDraw.ImageDraw) -> None:
    panel(draw, (18, 62, 382, 126), "PORTFOLIO SUMMARY")
    draw_text(draw, (34, 104), "$42,860", 20, bold=True, anchor="lm")
    draw_text(draw, (198, 101), "+1.8%", 14, fill=RED, bold=True, anchor="lm")
    draw_text(draw, (198, 116), "Today", 8, fill=GRAY, anchor="lm")
    panel(draw, (18, 142, 382, 246), "TOP MOVERS")
    rows = [("AAPL", "214.52", "+1.2%"), ("MSFT", "513.40", "-0.4%"), ("BTC", "118,240", "+2.7%")]
    for idx, (ticker, price, move) in enumerate(rows):
        y = 180 + idx * 23
        draw_text(draw, (34, y), ticker, 9, bold=True, anchor="lm")
        draw_text(draw, (170, y), price, 9, mono=True, anchor="rm")
        draw_text(draw, (350, y), move, 9, fill=RED if move.startswith("+") else BLACK, anchor="rm")
    draw_text(draw, (34, 236), "Delayed quotes | Not investment advice", 7, fill=GRAY, anchor="lm")


def draw_economic(draw: ImageDraw.ImageDraw) -> None:
    panel(draw, (18, 62, 382, 250), "ECONOMIC CALENDAR")
    rows = [
        ("08:30", "US CPI", "High", "Forecast 3.1%"),
        ("10:00", "EU GDP", "Medium", "Previous 0.4%"),
        ("21:45", "CN PMI", "High", "Consensus 50.2"),
        ("23:00", "FOMC Minutes", "High", "Watch USD"),
    ]
    for idx, (time, title, impact, detail) in enumerate(rows):
        y0 = 101 + idx * 36
        rect(draw, (30, y0, 370, y0 + 28), outline=RED if impact == "High" else LIGHT, fill=WHITE)
        draw_text(draw, (42, y0 + 14), time, 8, fill=RED if impact == "High" else BLACK, mono=True, anchor="lm")
        draw_text(draw, (100, y0 + 10), title, 9, bold=True, anchor="lm")
        draw_text(draw, (100, y0 + 22), detail, 7, fill=GRAY, anchor="lm")
        draw_text(draw, (358, y0 + 14), impact.upper(), 7, fill=RED if impact == "High" else GRAY, bold=True, anchor="rm")


@dataclass(frozen=True)
class Page:
    filename: str
    title: str
    icon: str
    draw_body: Callable[[ImageDraw.ImageDraw], None]


PAGES = [
    Page("ui-overview.png", "TODAY OVERVIEW", "overview", draw_overview),
    Page("ui-monthly.png", "MONTHLY OVERVIEW", "month", draw_monthly),
    Page("ui-weekly.png", "WEEKLY TIMELINE", "week", draw_weekly),
    Page("ui-world-clock.png", "WORLD CLOCK", "clock", draw_world_clock),
    Page("ui-news.png", "HEADLINES", "news", draw_news),
    Page("ui-weather.png", "WEATHER TODAY", "weather", draw_weather_expanded),
    Page("ui-portfolio.png", "PORTFOLIO", "portfolio", draw_portfolio),
    Page("ui-economic-calendar.png", "ECONOMIC CALENDAR", "economic", draw_economic),
]


def save_pages(out_dir: Path) -> list[Path]:
    out_dir.mkdir(parents=True, exist_ok=True)
    total = len(PAGES)
    paths: list[Path] = []
    for idx, page in enumerate(PAGES, start=1):
        image, draw = draw_shell(page.title, page.icon, idx, total)
        page.draw_body(draw)
        out_path = out_dir / page.filename
        image.save(out_path, optimize=True)
        paths.append(out_path)
    return paths


def save_weekly_weather_page(out_dir: Path) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    image, draw = draw_shell("WEEKLY WEATHER", "weather", 6, 8)
    draw_weekly_weather(draw)
    out_path = out_dir / "ui-weekly-weather.png"
    image.save(out_path, optimize=True)
    return out_path


def save_contact_sheet(paths: list[Path], out_dir: Path) -> Path:
    thumb_w, thumb_h = 800, 600
    cols = 4
    rows = 2
    gap = 40
    sheet = Image.new("RGB", (cols * thumb_w + (cols + 1) * gap, rows * thumb_h + (rows + 1) * gap), BG)
    for idx, path in enumerate(paths):
        image = Image.open(path)
        image = image.resize((thumb_w, thumb_h), Image.Resampling.LANCZOS)
        col = idx % cols
        row = idx // cols
        x = gap + col * (thumb_w + gap)
        y = gap + row * (thumb_h + gap)
        sheet.paste(image, (x, y))
    out_path = out_dir / "NM-EPD-420-UI-Prototype-Contact-Sheet.png"
    sheet.save(out_path, optimize=True)
    return out_path


def main() -> None:
    repo = Path(__file__).resolve().parents[1]
    out_dir = repo / "prototype"
    paths = save_pages(out_dir)
    save_contact_sheet(paths, out_dir)
    for path in paths:
        print(path)
    print(out_dir / "NM-EPD-420-UI-Prototype-Contact-Sheet.png")


if __name__ == "__main__":
    main()
