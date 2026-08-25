from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Inc" / "splash.h"

W, H = 320, 120

img = Image.new("RGB", (W, H), (255, 255, 255))
draw = ImageDraw.Draw(img)

font_paths = [
    "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
    "/System/Library/Fonts/Supplemental/Arial.ttf",
]

font_big = None
font_small = None

for p in font_paths:
    try:
        font_big = ImageFont.truetype(p, 27)
        font_small = ImageFont.truetype(p, 15)
        break
    except OSError:
        pass

if font_big is None:
    font_big = ImageFont.load_default()
    font_small = ImageFont.load_default()

title = "Vital Signs Monitor"
subtitle = "Starting..."

bbox = draw.textbbox((0, 0), title, font=font_big)
tw = bbox[2] - bbox[0]

draw.text(
    ((W - tw) // 2, 30),
    title,
    fill=(30, 70, 120),
    font=font_big
)

bbox = draw.textbbox((0, 0), subtitle, font=font_small)
sw = bbox[2] - bbox[0]

draw.text(
    ((W - sw) // 2, 76),
    subtitle,
    fill=(100, 100, 100),
    font=font_small
)

img.save(ROOT / "splash_preview.png")

data = []

for r, g, b in img.getdata():
    v = (
        ((r >> 3) << 11)
        | ((g >> 2) << 5)
        | (b >> 3)
    )
    data.append(v)

with OUT.open("w") as f:
    f.write("#ifndef SPLASH_H\n")
    f.write("#define SPLASH_H\n\n")
    f.write("#include <stdint.h>\n\n")

    f.write(f"const uint16_t SplashScreen[{W * H}] = {{\n")

    for i, v in enumerate(data):

        if i % 12 == 0:
            f.write("    ")

        f.write(f"0x{v:04X}")

        if i != len(data) - 1:
            f.write(", ")

        if i % 12 == 11:
            f.write("\n")

    f.write("\n};\n\n")
    f.write("#endif\n")

print("Splash generated")
