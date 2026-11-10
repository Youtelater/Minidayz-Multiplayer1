#!/usr/bin/env python3
"""Extract structural data from the original Construct 2 data.deobfuscated.js export.

Run from repository root:
  python tools/stage1/extract_construct.py data.deobfuscated.js --out src/generated/stage1
This script intentionally does not read romfs/, raw/, c2injt.js, or MP JSON files.
"""
import argparse, csv, json, re
from collections import Counter
from pathlib import Path

def load_export(path):
    text = Path(path).read_text(encoding="utf-8")
    # The deobfuscated export can contain whitespace after unary minus.
    text = re.sub(r'(?<![A-Za-z0-9_.])-\\s+(?=\\d)', '-', text)
    root = json.loads(text)
    if not isinstance(root, dict) or not isinstance(root.get("project"), list):
        raise ValueError("Not a recognized Construct project export")
    return root, root["project"]

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("source", nargs="?", default="data.deobfuscated.js")
    ap.add_argument("--out", default="src/generated/stage1")
    a=ap.parse_args()
    root,p=load_export(a.source); out=Path(a.out); out.mkdir(parents=True,exist_ok=True)
    (out/"construct_project_parsed.json").write_text(json.dumps(root,ensure_ascii=False,separators=(",",":")),encoding="utf-8")
    objects=p[3]; layouts=[]; instances=[]; layers=[]; animations=[]; image_refs=[]
    for li,L in enumerate(p[5]):
        first=len(instances); layer_records=[]
        for yi,layer in enumerate(L[6]):
            placed=layer[14] if len(layer)>14 and isinstance(layer[14],list) else []
            lf=len(instances)
            for ii,inst in enumerate(placed):
                if not isinstance(inst,list) or len(inst)<3 or not isinstance(inst[0],list): continue
                t=inst[0]; oi=inst[1]
                instances.append(dict(layout=li,layer=yi,object=oi,uid=inst[2],
                    x=t[0] if len(t)>0 else 0,y=t[1] if len(t)>1 else 0,
                    z=t[2] if len(t)>2 else 0,width=t[3] if len(t)>3 else 0,
                    height=t[4] if len(t)>4 else 0,angle=t[5] if len(t)>5 else 0,
                    opacity=t[6] if len(t)>6 else 1,scale_x=t[7] if len(t)>7 else 1,
                    origin_x=t[8] if len(t)>8 else .5,origin_y=t[9] if len(t)>9 else .5))
            layers.append(dict(layout=li,index=yi,name=layer[0],first_instance=lf,
                instance_count=len(instances)-lf,visible=bool(layer[3]) if len(layer)>3 else True))
            layer_records.append(dict(index=yi,name=layer[0],first_instance=lf,instance_count=len(instances)-lf))
        layouts.append(dict(index=li,name=L[0],width=L[1],height=L[2],event_sheet=L[4],
                            first_instance=first,instance_count=len(instances)-first,layers=layer_records))
    for oi,o in enumerate(objects):
        for anim in (o[7] if len(o)>7 and isinstance(o[7],list) else []):
            if not isinstance(anim,list) or not anim: continue
            frames=anim[7] if len(anim)>7 and isinstance(anim[7],list) else []
            animations.append(dict(object=oi,name=anim[0],frames=len(frames),
                loop=bool(anim[2]) if len(anim)>2 else False,speed=anim[3] if len(anim)>3 else 0))
            for frame in frames:
                if isinstance(frame,list) and frame and isinstance(frame[0],str):
                    image_refs.append(dict(object=oi,path=frame[0]))
    event_sheets=[]
    for sheet in p[6]:
        counts=Counter(str(e[0]) for e in sheet[1] if isinstance(e,list) and e)
        event_sheets.append(dict(name=sheet[0],top_level_blocks=len(sheet[1]),block_types=dict(counts)))
    audio=[dict(filename=x[0],source_bytes=x[1]) for x in p[7] if isinstance(x,list) and len(x)>1]
    manifest=dict(source=Path(a.source).name,project_sections=len(p),object_definitions=len(objects),
        layouts=layouts,layers=len(layers),instances=len(instances),animations=len(animations),
        image_references=len(image_refs),audio_resources=len(audio),event_sheets=event_sheets)
    (out/"stage1_manifest.json").write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding="utf-8")
    for name,rows in [("instances.csv",instances),("animations.csv",animations),("image_refs.csv",image_refs),("audio_resources.csv",audio)]:
        if rows:
            with (out/name).open("w",newline="",encoding="utf-8") as f:
                w=csv.DictWriter(f,fieldnames=list(rows[0])); w.writeheader(); w.writerows(rows)
    print(json.dumps({k:v for k,v in manifest.items() if k!="layouts"},indent=2))
if __name__=="__main__": main()
