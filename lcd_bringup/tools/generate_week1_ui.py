from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT.parent / "assets"
OUT = ROOT / "Inc" / "RGB565_480x272.h"

W, H = 480, 272

# --------------------------------------------------
# Base UI: white background
# --------------------------------------------------

base = Image.new("RGB", (W, H), (255, 255, 255))

gauge = Image.open(
    ASSETS / "Gauge" / "Gauge.png"
).convert("RGBA")

thermo = Image.open(
    ASSETS / "Thermometer" / "Thermometer.png"
).convert("RGBA")

gauge.thumbnail((135, 135), Image.Resampling.LANCZOS)
thermo.thumbnail((95, 180), Image.Resampling.LANCZOS)


def paste_centered(dst, img, cx, cy):
    x = int(cx - img.width / 2)
    y = int(cy - img.height / 2)
    dst.paste(img, (x, y), img)


# Gauge
paste_centered(base, gauge, 250, 130)

# Fixed gauge needle
needle_draw = ImageDraw.Draw(base)

gauge_cx = 250
gauge_cy = 130

# Needle points toward upper-right
needle_draw.line(
    (gauge_cx, gauge_cy, 285, 100),
    fill=(210, 40, 40),
    width=5
)

# Needle center cap
needle_draw.ellipse(
    (
        gauge_cx - 6,
        gauge_cy - 6,
        gauge_cx + 6,
        gauge_cy + 6
    ),
    fill=(70, 70, 70)
)


# Thermometer
paste_centered(base, thermo, 405, 130)

# --------------------------------------------------
# Static HR text for Week 1 demo
# --------------------------------------------------

draw = ImageDraw.Draw(base)

font_paths = [
    "/System/Library/Fonts/Supplemental/Arial.ttf",
    "/Library/Fonts/Arial.ttf",
]

font = None

for path in font_paths:
    try:
        font = ImageFont.truetype(path, 28)
        break
    except OSError:
        pass

if font is None:
    font = ImageFont.load_default()

draw.text(
    (34, 215),
    "HR: 76",
    fill=(30, 30, 30),
    font=font
)

# --------------------------------------------------
# RGB565 converter for base layer
# --------------------------------------------------

def rgb565(img):
    data = []

    for r, g, b in img.convert("RGB").getdata():
        value = (
            ((r >> 3) << 11)
            | ((g >> 2) << 5)
            | (b >> 3)
        )

        data.append(value)

    return data


# --------------------------------------------------
# ARGB4444 converter for transparent heart layer
# --------------------------------------------------

def argb4444(img):
    data = []

    for r, g, b, a in img.convert("RGBA").getdata():
        value = (
            ((a >> 4) << 12)
            | ((r >> 4) << 8)
            | ((g >> 4) << 4)
            | (b >> 4)
        )

        data.append(value)

    return data


# --------------------------------------------------
# Heart animation frames
# --------------------------------------------------

heart_frames = []

for i in range(1, 9):

    heart = Image.open(
        ASSETS / "Heart" / "Animation" / f"Heart{i}.png"
    ).convert("RGBA")

    heart.thumbnail((115, 115), Image.Resampling.LANCZOS)

    # Transparent 140x140 layer
    tile = Image.new(
        "RGBA",
        (140, 140),
        (0, 0, 0, 0)
    )

    x = (140 - heart.width) // 2
    y = (140 - heart.height) // 2

    tile.alpha_composite(heart, (x, y))

    tile.save(ROOT / f"heart_frame_{i}.png")

    heart_frames.append(argb4444(tile))


# --------------------------------------------------
# Preview of final result
# --------------------------------------------------

preview = base.convert("RGBA")

heart_preview = Image.open(
    ROOT / "heart_frame_1.png"
).convert("RGBA")

preview.alpha_composite(
    heart_preview,
    (20, 60)
)

preview.save(ROOT / "week1_preview.png")


# --------------------------------------------------
# Generate C header
# --------------------------------------------------

with OUT.open("w") as f:

    f.write("#ifndef RGB565_480X272_H\n")
    f.write("#define RGB565_480X272_H\n\n")
    f.write("#include <stdint.h>\n\n")

    def write_array(name, data):

        f.write(
            f"const uint16_t {name}[{len(data)}] = {{\n"
        )

        for idx, value in enumerate(data):

            if idx % 12 == 0:
                f.write("    ")

            f.write(f"0x{value:04X}")

            if idx != len(data) - 1:
                f.write(", ")

            if idx % 12 == 11:
                f.write("\n")

        f.write("\n};\n\n")

    write_array(
        "RGB565_480x272",
        rgb565(base)
    )

    for i, frame in enumerate(heart_frames, 1):
        write_array(
            f"HeartFrame{i}",
            frame
        )

    f.write("#endif\n")

print("Week 1 UI generated successfully")
print("Preview:", ROOT / "week1_preview.png")
