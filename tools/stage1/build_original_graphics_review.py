#!/usr/bin/env python3
"""Visual contact sheets of *all* original assets/gfx PNG files.

This is a review artifact, NOT a replacement for sprite frames. Do not
put preview spritesheets in romfs/gfx or render them as in-game art.
"""
from __future__ import annotations
import argparse
import csv
import io
import math
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageOps

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "assets" / "gfx"
DEST = ROOT / "data" / "graphics_review"
COLS, ROWS = 16, 10
CELL_W, CELL_H = 94, 83
PER_PAGE = COLS * ROWS

def category(name: str) -> str:
    n = name.lower()
    if "menu" in n or n.startswith(("tiledbgmenu", "sub_menu", "amenu_")):
        return "01-menu"
    if n.startswith(("gui_", "ninepatch_", "minimap_", "minimap", "bg_button")):
        return "02-interface"
    if n.startswith(("player_", "playerbase", "hand_")):
        return "03-player"
    if "tilemap" in n or n.startswith(("ground_", "tree_", "water_", "bridge_",
                                       "bunker_", "tiledbg_", "fake_water")):
        return "04-world"
    if n.startswith(("b_", "ee_", "car_", "fence", "bush", "road", "forest")):
        return "05-structures"
    if n.startswith(("zed_", "npc_", "bot_", "bandit_", "dog_", "wolf_", "bear_")):
        return "06-enemies"
    if n.startswith(("ach_", "perk_", "item_")):
        return "07-achievements-items"
    return "08-other-original-assets"

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--print-b64", type=int, default=0,
                        help="Print this many JPG contact sheets as base64 in CI logs for direct visual audit")
    args = parser.parse_args()
    import base64
    files = list(SOURCE.glob("*.png"))
    if not files:
        raise SystemExit("assets/gfx has no original PNG images")
    files.sort(key=lambda p: (category(p.name), p.name.casefold(), p.name))
    DEST.mkdir(parents=True, exist_ok=True)
    font = ImageFont.load_default()
    rows = []
    counts = {}
    sheets = []
    for page in range(math.ceil(len(files) / PER_PAGE)):
        canvas = Image.new("RGB", (COLS * CELL_W, ROWS * CELL_H), "#22272a")
        painter = ImageDraw.Draw(canvas)
        for local, f in enumerate(files[page * PER_PAGE:(page + 1) * PER_PAGE]):
            i = page * PER_PAGE + local
            col, row = local % COLS, local // COLS
            x, y = col * CELL_W, row * CELL_H
            painter.rectangle((x+2,y+2,x+CELL_W-3,y+CELL_H-3), fill="#3a4142")
            try:
                with Image.open(f) as original:
                    original.load()
                    width,height = original.size
                    rgba = original.convert("RGBA")
                    thumb = ImageOps.contain(rgba, (CELL_W - 12, CELL_H - 27))
                    bg = Image.new("RGBA", thumb.size, "#737373")
                    bg.alpha_composite(thumb)
                    px = x + (CELL_W-thumb.width)//2
                    py = y + 3 + (CELL_H-27-thumb.height)//2
                    canvas.paste(bg.convert("RGB"), (px, py))
                    alpha = rgba.getchannel("A").getextrema()
                    transparent = alpha[0] < 255
                    rows.append([i,category(f.name),f.name,width,height,
                                original.mode,int(transparent),f.stat().st_size,
                                page,f"contact_{page:02d}.jpg"])
            except Exception as exc:
                raise RuntimeError(f"Cannot decode original PNG {f}: {exc}") from exc
            counts[category(f.name)] = counts.get(category(f.name),0)+1
            label=f.name.replace("-sheet0.png","").replace(".png","")
            painter.text((x+4,y+CELL_H-20),f"{i:04d} {label[:12]}",font=font,fill="#f1efdc")
        output=DEST/f"contact_{page:02d}.jpg"
        canvas.save(output, quality=78,optimize=True)
        sheets.append(output)
    with (DEST/"all_original_images.csv").open("w",newline="",encoding="utf-8") as fh:
        writer=csv.writer(fh)
        writer.writerow(["index","category","original_png","width","height","mode",
                         "has_transparency","png_bytes","contact_page","contact_sheet"])
        writer.writerows(rows)
    with (DEST/"README.txt").open("w",encoding="utf-8") as fh:
        fh.write(f"Original MiniDAYZ graphics review: {len(files)} PNG images, {len(sheets)} contact sheets.\\n")
        fh.write("Every thumbnail is sampled from assets/gfx unchanged; index CSV lists the exact filename and pixel size.\\n")
        fh.write("Use these contact sheets ONLY for comparison; they are not game textures.\\n")
        for key,count in sorted(counts.items()):
            fh.write(f"{key}: {count} images\\n")
    print(f"ORIGINAL GFX VISUAL AUDIT: {len(files)} images decoded, {len(sheets)} contact sheets.")
    print("CATEGORIES:",dict(sorted(counts.items())))
    for page in range(min(args.print_b64,len(sheets))):
        content=base64.b64encode(sheets[page].read_bytes()).decode("ascii")
        print(f"BEGIN_CONTACT_SHEET_{page:02d}")
        print(content)
        print(f"END_CONTACT_SHEET_{page:02d}")
if __name__ == "__main__":
    main()
