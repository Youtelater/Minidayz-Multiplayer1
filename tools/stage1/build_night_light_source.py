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
runtime=(ROOT/"c2runtime.js").read_text(encoding="utf-8")
assert "case 8:b.Qc=d.ZERO;b.Jc=d.ONE_MINUS_SRC_ALPHA" in runtime
assert "front *= back.a;" in runtime[runtime.index("overlay:{src:"):]
assert objects[663][12]==[["overlay","Overlay"]]
# The GUI Color_effect is separate from the Map night layer. Validate
# the source Fade behaviour and both creation events before compiling.
gui=next(l for l in source[5] if l[0]=="GUI_layout")
color_layer=next(l for l in gui[6] if l[0]=="Color_effect")
twilight=next(i for i in color_layer[14] if i[1]==971)
assert twilight[4][0]==[1,65,0,25,1]
assert len(objects[971][7][0][7])==2
def find_event(events,uid):
    for e in events:
        if not isinstance(e,list) or not e: continue
        if e[0]==0 and len(e)>6 and e[4]==uid:return e
        if e[0]==0 and len(e)>7:
            found=find_event(e[7],uid)
            if found is not None:return found
    return None
events=next(s[1] for s in source[6] if s[0]=="Game_events")
for uid,minute in ((316761403775120,1110),(347882179964897,330)):
    event=find_event(events,uid)
    assert event is not None and event[5][0][9]==[[11,"Timer_ALL"],[8,0],[7,[0,minute]]]
    assert any(a[0]==-1 and a[1]==94 and a[5][0]==[4,971]
               and a[5][1]==[5,[2,"Color_effect"]] for a in event[6])
# The root bunker and FinalStand transitions deliberately disable the
# outdoor night effect, rather than applying a guessed dark overlay.
for entry_uid, hide_uid in ((305470436850518,453618219439986),
                            (113177369442063,830000454019733)):
    entry=find_event(events,entry_uid)
    hide=find_event(events,hide_uid)
    assert entry is not None and hide is not None
    for name in ("inRaid","inBunker"):
        assert any(a[0]==-1 and a[1]==41 and a[5]==[[11,name],[7,[0,1]]]
                   for a in entry[6]), (entry_uid,name)
    assert any(a[0]==-1 and a[1]==41 and a[5]==[[11,"Day_mode"],[7,[0,1]]]
               for a in hide[6]), hide_uid
    assert any(a[0]==-1 and a[1]==162 and a[5]==[[5,[2,"night"]],[3,0]]
               for a in hide[6]), hide_uid
    assert any(a[0]==-1 and a[1]==74 and a[5]==[[1,[2,"night_force_texture"]],[3,0]]
               for a in hide[6]), hide_uid
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
