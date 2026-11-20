#!/usr/bin/env python3
"""Extract the ACTUAL wooden button frame from the original Construct sprite sheet.

menu_btn_wood-sheet0.png is a 256x256 ATLAS of multiple button states.
Displaying the entire atlas as one rectangular button creates the multicolor
stripes seen in Manic Emu. Never resize the entire source sheet into a button.
"""
from pathlib import Path
import hashlib
import subprocess
import tempfile
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/"assets/gfx/menu_btn_wood-sheet0.png"
OUT=ROOT/"romfs/gfx/menu_btn_wood_single.t3x"
STAMP=ROOT/"romfs/gfx/menu_btn_wood_single.sha256"
# First complete, unselected wooden button (source sheet frame zero).
# These coordinates are in the ORIGINAL 256x256 source atlas.
RECT=(3,4,161,39)

def main():
    with Image.open(SOURCE) as original:
        original.load()
        assert original.size==(256,256),f"Original menu atlas changed: {original.size}"
        cropped=original.convert("RGBA").crop(RECT)
        assert cropped.size==(158,35)
        assert cropped.getbbox() is not None
        digest=hashlib.sha256(original.tobytes()+bytes(RECT)).hexdigest()
    OUT.parent.mkdir(parents=True,exist_ok=True)
    if OUT.is_file() and OUT.stat().st_size>32 and STAMP.is_file() and STAMP.read_text().strip()==digest:
        print("Original single-frame menu button already converted")
        return
    with tempfile.TemporaryDirectory(prefix="minidayz_menu_") as dirname:
        frame=Path(dirname)/"menu_button_source.png"
        cropped.save(frame)
        result=subprocess.run(["tex3ds","-o",str(OUT),str(frame)],check=False)
        if result.returncode!=0 or not OUT.is_file() or OUT.stat().st_size<32:
            raise SystemExit("Failed converting the original wood button frame")
    STAMP.write_text(digest+"\n",encoding="ascii")
    print("Extracted original wooden button frame without atlas stripes")

if __name__=="__main__":
    main()
