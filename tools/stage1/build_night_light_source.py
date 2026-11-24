#!/usr/bin/env python3
"""Extract the exact original Sprite240 color and Sprite663 alpha frames.

Root Map/night uses blend mode8 for the lamp: destination-out
(c2runtime.js ob case8, ZERO / ONE_MINUS_SRC_ALPHA). No redrawn light
shapes or hand-cropped sheets; frame rectangles come from the original.
"""
import json,re
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[2]
source=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
    (ROOT/"data.deobfuscated.js").read_text(encoding="utf-8-sig")))["project"]
objects=source[3]
layout=next(l for l in source[5] if l[0]=="Map")
night=next(l for l in layout[6] if l[0]=="night")
assert night[9] is True # force own texture
lamp=next(i for i in night[14] if i[1]==663)
assert lamp[0][10]==8 and lamp[0][7]==0.5
assert "case 8:b.Qc=d.ZERO;b.Jc=d.ONE_MINUS_SRC_ALPHA" in (ROOT/"c2runtime.js").read_text(encoding="utf-8")
def frame_image(frame):
    x,y,w,h=map(int,frame[2:6])
    path=ROOT/"assets/gfx"/Path(frame[0]).name
    with Image.open(path) as image:
        assert x>=0 and y>=0 and x+w<=image.width and y+h<=image.height, frame
        return image.convert("RGBA").crop((x,y,x+w,y+h))
overlay=frame_image(objects[240][7][0][7][0])
colors=list(overlay.getdata())
assert len(set(colors))==1 and colors[0][3]==255,colors
r,g,b,a=colors[0]
out=["#pragma once","#include <cstdint>","namespace minidayz::world_source {",
     f"inline constexpr std::uint32_t kSourceNightRgb=0x{r:02x}{g:02x}{b:02x}00u;",
     "struct SourceNightFrame {unsigned w,h;const std::uint8_t* alpha;};"]
frames=objects[663][7][0][7]
assert len(frames)==2
for n,frame in enumerate(frames):
    im=frame_image(frame)
    alpha=im.getchannel("A").tobytes()
    out.append(f"inline constexpr std::uint8_t kSourceNightLamp{n}[]={{")
    for start in range(0,len(alpha),32):
        out.append(",".join(map(str,alpha[start:start+32]))+",")
    out.append("};")
out.append("inline constexpr SourceNightFrame kSourceNightLampFrames[]={")
for n,frame in enumerate(frames):
    out.append(f"{{{frame[4]},{frame[5]},kSourceNightLamp{n}}},")
out+=["};","} // namespace minidayz::world_source",""]
dest=ROOT/"include/minidayz/generated/source_night_light.hpp"
dest.parent.mkdir(parents=True,exist_ok=True)
dest.write_text("\n".join(out),encoding="utf-8")
print("PASS original night color and lamp frame alpha masks")
