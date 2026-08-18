from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT.parent / "assets"

BASE_SOURCE = ROOT / "ui_preview.png"
OUT = ROOT / "Inc" / "RGB565_480x272.h"

W, H = 480, 272

# -------------------------
# BASE UI
# -------------------------

base = Image.open(BASE_SOURCE).convert("RGB")

# Sample the real background color
bg = base.getpixel((0, 0))

# Remove the old static heart from the base image
draw = ImageDraw.Draw(base)
draw.rectangle((15, 55, 165, 215), fill=bg)

base.save(ROOT / "ui_base_preview.png")


def rgb565(img):
    data = []

    for r, g, b in img.getdata():
        value = (
            ((r >> 3) << 11) |
            ((g >> 2) << 5) |
            (b >> 3)
        )
        data.append(value)

    return data


# -------------------------
# HEART FRAMES
# ARGB4444
# -------------------------

def argb4444(img):
    data = []

    for r, g, b, a in img.getdata():

        value = (
            ((a >> 4) << 12) |
            ((r >> 4) << 8) |
            ((g >> 4) << 4) |
            (b >> 4)
        )

        data.append(value)

    return data


heart_frames = []

for i in range(1, 9):

    heart = Image.open(
        ASSETS / "Heart" / "Animation" / f"Heart{i}.png"
    ).convert("RGBA")

    heart.thumbnail(
        (115, 115),
        Image.Resampling.LANCZOS
    )

    # Fully transparent layer
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


# -------------------------
# WRITE HEADER
# -------------------------

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

print("Generated:")
print(" - RGB565 base UI")
print(" - 8 transparent ARGB4444 heart frames")
