from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "assets"
OUT = Path(__file__).resolve().parents[1] / "Inc" / "RGB565_480x272.h"
PREVIEW = Path(__file__).resolve().parents[1] / "ui_preview.png"

WIDTH = 480
HEIGHT = 272

screen = Image.new("RGB", (WIDTH, HEIGHT), (0, 70, 140))

heart = Image.open(
    ASSETS / "Heart" / "HeartForeground.png"
).convert("RGBA")

gauge = Image.open(
    ASSETS / "Gauge" / "Gauge.png"
).convert("RGBA")

thermometer = Image.open(
    ASSETS / "Thermometer" / "Thermometer.png"
).convert("RGBA")

heart.thumbnail((120, 120), Image.Resampling.LANCZOS)
gauge.thumbnail((130, 130), Image.Resampling.LANCZOS)
thermometer.thumbnail((85, 150), Image.Resampling.LANCZOS)

def paste_centered(img, cx, cy):
    x = int(cx - img.width / 2)
    y = int(cy - img.height / 2)
    screen.paste(img, (x, y), img)

paste_centered(heart, 90, 136)
paste_centered(gauge, 245, 136)
paste_centered(thermometer, 400, 136)

screen.save(PREVIEW)

pixels = list(screen.getdata())

with OUT.open("w") as f:
    f.write("#ifndef RGB565_480X272_H\n")
    f.write("#define RGB565_480X272_H\n\n")
    f.write("#include <stdint.h>\n\n")
    f.write("const uint16_t RGB565_480x272[480 * 272] = {\n")

    for i, (r, g, b) in enumerate(pixels):
        value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

        if i % 12 == 0:
            f.write("    ")

        f.write(f"0x{value:04X}")

        if i != len(pixels) - 1:
            f.write(", ")

        if i % 12 == 11:
            f.write("\n")

    f.write("\n};\n\n")
    f.write("#endif\n")

print(f"Generated: {OUT}")
print(f"Preview:   {PREVIEW}")
