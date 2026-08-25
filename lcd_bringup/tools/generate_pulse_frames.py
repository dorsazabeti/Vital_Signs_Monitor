from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "ui_preview.png"
OUT = ROOT / "Inc" / "RGB565_480x272.h"

base = Image.open(SRC).convert("RGB")
pulse = base.copy()

# محدوده تقریبی قلب روی UI فعلی
box = (25, 70, 155, 200)
heart_area = base.crop(box)

bigger = heart_area.resize(
    (145, 145),
    Image.Resampling.LANCZOS
)

x = 90 - bigger.width // 2
y = 135 - bigger.height // 2

pulse.paste(bigger, (x, y))

def rgb565(img):
    out = []
    for r, g, b in img.getdata():
        v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        out.append(v)
    return out

frames = [
    ("RGB565_480x272", rgb565(base)),
    ("RGB565_480x272_Pulse", rgb565(pulse)),
]

with OUT.open("w") as f:
    f.write("#ifndef RGB565_480X272_H\n")
    f.write("#define RGB565_480X272_H\n\n")
    f.write("#include <stdint.h>\n\n")

    for name, pixels in frames:
        f.write(f"const uint16_t {name}[480 * 272] = {{\n")

        for i, value in enumerate(pixels):
            if i % 12 == 0:
                f.write("    ")

            f.write(f"0x{value:04X}")

            if i != len(pixels) - 1:
                f.write(", ")

            if i % 12 == 11:
                f.write("\n")

        f.write("\n};\n\n")

    f.write("#endif\n")

print("Generated two UI frames")
