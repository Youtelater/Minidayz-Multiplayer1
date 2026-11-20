#!/usr/bin/env python3
"""Extract a REAL Construct menu button frame, never the entire sprite atlas.

The original Construct export stores the precise rectangle for every animation
frame. Do not hand-guess pixels inside menu_btn_wood-sheet0.png. Generated art
lives in romfs/gfx; the original PNG is read-only in assets/gfx.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import tempfile
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "assets/gfx/menu_btn_wood-sheet0.png"
PARSED = ROOT / "data/stage1/construct_project_parsed.json"
OUT = ROOT / "romfs/gfx/menu_btn_wood_single.t3x"
STAMP = ROOT / "romfs/gfx/menu_btn_wood_single.sha256"


def first_source_frame():
    if not PARSED.is_file():
        raise SystemExit("Missing parsed original Construct data for menu sprite")
    cleaned = re.sub(r"(?<![A-Za-z0-9_.])-\\s+(?=\\d)", "-",
                     PARSED.read_text(encoding="utf-8-sig"))
    objects = json.loads(cleaned)["project"][3]
    for object_id, obj in enumerate(objects):
        animations = obj[7] if isinstance(obj, list) and len(obj) > 7 and isinstance(obj[7], list) else []
        for animation_id, animation in enumerate(animations):
            frames = animation[7] if isinstance(animation, list) and len(animation) > 7 and isinstance(animation[7], list) else []
            for frame_id, frame in enumerate(frames):
                if not isinstance(frame, list) or len(frame) < 6 or not isinstance(frame[0], str):
                    continue
                if Path(frame[0].replace("\\\\", "/").replace("\\", "/")).name != SOURCE.name:
                    continue
                x, y, w, h = (int(frame[i]) for i in (2, 3, 4, 5))
                if w <= 0 or h <= 0:
                    raise SystemExit(f"Invalid original menu button frame {object_id}:{animation_id}:{frame_id}")
                return (x,y,w,h), (object_id,animation_id,frame_id)
    raise SystemExit("Construct original has no menu_btn_wood sprite frame; refusing guessed crop")


def main():
    (x,y,w,h), frame_id = first_source_frame()
    with Image.open(SOURCE) as original:
        original.load()
        original_rgba = original.convert("RGBA")
        left,top,right,bottom = max(x,0),max(y,0),min(x+w,original.width),min(y+h,original.height)
        if right <= left or bottom <= top:
            raise SystemExit("Source menu button frame is completely outside PNG")
        cropped = Image.new("RGBA",(w,h),(0,0,0,0))
        cropped.paste(original_rgba.crop((left,top,right,bottom)),(left-x,top-y))
        if cropped.getbbox() is None:
            raise SystemExit("Original Construct menu button frame has no visible artwork")
        digest=hashlib.sha256(original.tobytes()+
                              json.dumps((x,y,w,h,frame_id)).encode("utf-8")).hexdigest()
    OUT.parent.mkdir(parents=True,exist_ok=True)
    if (OUT.is_file() and OUT.stat().st_size>32 and STAMP.is_file()
            and STAMP.read_text().strip()==digest):
        print("Source-verified original wooden button frame already converted:",frame_id)
        return
    with tempfile.TemporaryDirectory(prefix="minidayz_menu_") as dirname:
        frame=Path(dirname)/"menu_button_source.png"
        cropped.save(frame)
        result=subprocess.run(["tex3ds","-o",str(OUT),str(frame)],check=False)
        if result.returncode!=0 or not OUT.is_file() or OUT.stat().st_size<32:
            raise SystemExit("Failed converting original source wood button frame")
    STAMP.write_text(digest+"\\n",encoding="ascii")
    print("Extracted exact original Construct wooden button frame",
          frame_id,"rectangle",(x,y,w,h),"without atlas stripes")


if __name__=="__main__":
    main()
