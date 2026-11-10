#!/usr/bin/env python3
"""Extract the supplied original Construct export into machine-readable and C++ data.
Usage: python tools/stage1/extract_construct.py data.deobfuscated.js --out src/generated/stage1
Does not read romfs/, raw/, c2injt.js, or any MP JSON file.
"""
import argparse,csv,json,re
from collections import Counter
from pathlib import Path

def load(path):
    s=Path(path).read_text(encoding="utf-8")
    s=re.sub(r'(?<![A-Za-z0-9_.])-\s+(?=\d)','-',s)
    root=json.loads(s)
    if not isinstance(root,dict) or not isinstance(root.get("project"),list):
        raise ValueError("Not a Construct project export")
    return root,root["project"]

def q(x): return json.dumps(str(x),ensure_ascii=False)
def cf(x):
    z=format(float(x),".9g")
    if "." not in z and "e" not in z.lower(): z+=".0"
    return z+"f"

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("source",nargs="?",default="data.deobfuscated.js")
    ap.add_argument("--out",default="src/generated/stage1")
    a=ap.parse_args(); root,p=load(a.source); out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
    objects=p[3]; layouts=[];layers=[];instances=[];animations=[];image_refs=[]
    for li,L in enumerate(p[5]):
        first=len(instances); layer_defs=[]
        for yi,layer in enumerate(L[6]):
            lf=len(instances); placed=layer[14] if len(layer)>14 and isinstance(layer[14],list) else []
            for inst in placed:
                if not isinstance(inst,list) or len(inst)<3 or not isinstance(inst[0],list): continue
                t=inst[0]; oi=inst[1]
                instances.append(dict(layout=li,layer=yi,object=oi,uid=inst[2],
                    x=t[0] if len(t)>0 else 0,y=t[1] if len(t)>1 else 0,z=t[2] if len(t)>2 else 0,
                    width=t[3] if len(t)>3 else 0,height=t[4] if len(t)>4 else 0,
                    angle=t[5] if len(t)>5 else 0,opacity=t[6] if len(t)>6 else 1,
                    scale_x=t[7] if len(t)>7 else 1,origin_x=t[8] if len(t)>8 else .5,
                    origin_y=t[9] if len(t)>9 else .5))
            layers.append(dict(layout=li,index=yi,name=layer[0],first=len(instances)-(len(instances)-lf),
                count=len(instances)-lf,visible=bool(layer[3]) if len(layer)>3 else True,
                parallax_x=layer[6] if len(layer)>6 else 1,parallax_y=layer[7] if len(layer)>7 else 1))
            layer_defs.append(dict(index=yi,name=layer[0],first=lf,count=len(instances)-lf))
        layouts.append(dict(index=li,name=L[0],width=L[1],height=L[2],event_sheet=L[4],
            first=first,count=len(instances)-first,layers=layer_defs))
    for oi,o in enumerate(objects):
        for an in (o[7] if len(o)>7 and isinstance(o[7],list) else []):
            if not isinstance(an,list) or not an: continue
            frames=an[7] if len(an)>7 and isinstance(an[7],list) else []
            animations.append(dict(object=oi,name=an[0],frames=len(frames),
                speed=an[3] if len(an)>3 else 0,loop=bool(an[2]) if len(an)>2 else False))
            for fr in frames:
                if isinstance(fr,list) and fr and isinstance(fr[0],str):
                    image_refs.append(dict(object=oi,path=fr[0]))
    sheets=[]; events=[]; variables=[]; groups=[]; cond=Counter(); acts=Counter()
    def walk(items,si,name,parent=-1,depth=0):
        for e in items:
            if not isinstance(e,list) or not e: continue
            if e[0]==1:
                variables.append(dict(sheet=si,name=e[1],initial=e[3] if len(e)>3 else None));continue
            if e[0]!=0: continue
            cs=e[5] if len(e)>5 and isinstance(e[5],list) else []
            ac=e[6] if len(e)>6 and isinstance(e[6],list) else []
            sub=e[7] if len(e)>7 and isinstance(e[7],list) else []
            if isinstance(e[1],list) and len(e[1])>1 and isinstance(e[1][1],str):
                groups.append(dict(sheet=si,name=e[1][1],uid=e[4] if len(e)>4 else 0))
            for x in cs:
                if isinstance(x,list) and len(x)>1 and isinstance(x[1],int):cond[x[1]]+=1
            for x in ac:
                if isinstance(x,list) and len(x)>1 and isinstance(x[1],int):acts[x[1]]+=1
            idx=len(events);events.append(dict(index=idx,sheet=si,parent=parent,depth=depth,
                uid=e[4] if len(e)>4 else 0,conditions=cs,actions=ac,subevents=len(sub)))
            walk(sub,si,name,idx,depth+1)
    for si,(name,entries) in enumerate(p[6]):
        start=len(events);vstart=len(variables);walk(entries,si,name)
        sheets.append(dict(index=si,name=name,top_level_entries=len(entries),
            event_nodes=len(events)-start,variables=len(variables)-vstart,first_event=start))
    audio=[dict(filename=x[0],bytes=x[1]) for x in p[7] if isinstance(x,list) and len(x)>1]
    (out/"construct_project_parsed.json").write_text(json.dumps(root,ensure_ascii=False,separators=(",",":")),encoding="utf-8")
    (out/"event_sheet_index.json").write_text(json.dumps(dict(sheets=sheets,events=events,variables=variables,groups=groups,
        condition_type_counts=cond,action_type_counts=acts),ensure_ascii=False,separators=(",",":")),encoding="utf-8")
    manifest=dict(source=Path(a.source).name,source_bytes=Path(a.source).stat().st_size,
        project_sections=len(p),version=p[16],canvas=[p[10],p[11]],objects=len(objects),
        layouts=layouts,layers=len(layers),instances=len(instances),animations=len(animations),
        image_frame_references=len(image_refs),audio_resources=len(audio),event_sheets=sheets,
        event_nodes=len(events),variables=len(variables),groups=len(groups),
        condition_command_types=len(cond),action_command_types=len(acts),
        condition_type_counts=cond,action_type_counts=acts)
    (out/"stage1_manifest.json").write_text(json.dumps(manifest,ensure_ascii=False,indent=2,default=dict),encoding="utf-8")
    for fn,rows in [("instances.csv",instances),("animations.csv",animations),("image_refs.csv",image_refs),("audio_resources.csv",audio)]:
        if rows:
            with (out/fn).open("w",newline="",encoding="utf-8") as f:
                w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
    h=["#pragma once","#include <cstdint>","#include <cstddef>","namespace minidayz::stage1 {",
       "struct ObjectDef{std::uint16_t index;std::uint8_t type;const char* name;};",
       "struct LayoutDef{const char* name;float width,height;const char* event_sheet;std::uint32_t first,count;};",
       "struct LayerDef{std::uint16_t layout,index;const char* name;std::uint32_t first,count;bool visible;float px,py;};",
       "struct InstanceDef{std::uint16_t layout,layer,object;std::uint64_t uid;float x,y,z,w,h,angle,opacity,scale_x,ox,oy;};",
       "struct AnimationDef{std::uint16_t object;const char* name;std::uint32_t frames;float speed;bool loop;};",
       "struct AudioDef{const char* filename;std::uint64_t bytes;};",
       "struct EventSheetDef{const char* name;std::uint32_t first,count,variables;};",
       "struct CommandCount{std::uint16_t id;std::uint32_t count;};",
       f"inline constexpr std::size_t kObjectCount={len(objects)},kLayoutCount={len(layouts)},kLayerCount={len(layers)},kInstanceCount={len(instances)},kAnimationCount={len(animations)},kAudioCount={len(audio)},kEventSheetCount={len(sheets)},kEventCount={len(events)},kVariableCount={len(variables)},kGroupCount={len(groups)};",
       "inline constexpr ObjectDef kObjects[]={"]
    for i,o in enumerate(objects):h.append(f"{{{i},{o[1]},{q(o[0])}}},")
    h+=["};","inline constexpr LayoutDef kLayouts[]={"]
    for x in layouts:h.append(f"{{{q(x['name'])},{cf(x['width'])},{cf(x['height'])},{q(x['event_sheet'])},{x['first']},{x['count']}}},")
    h+=["};","inline constexpr LayerDef kLayers[]={"]
    for x in layers:h.append(f"{{{x['layout']},{x['index']},{q(x['name'])},{x['first']},{x['count']},{str(x['visible']).lower()},{cf(x['parallax_x'])},{cf(x['parallax_y'])}}},")
    h+=["};","inline constexpr InstanceDef kInstances[]={"]
    for x in instances:h.append("{"+",".join([str(x[k]) for k in ("layout","layer","object","uid")]+[cf(x[k]) for k in ("x","y","z","width","height","angle","opacity","scale_x","origin_x","origin_y")])+"},")
    h+=["};","inline constexpr AnimationDef kAnimations[]={"]
    for x in animations:h.append(f"{{{x['object']},{q(x['name'])},{x['frames']},{cf(x['speed'])},{str(x['loop']).lower()}}},")
    h+=["};","inline constexpr AudioDef kAudio[]={"]
    for x in audio:h.append(f"{{{q(x['filename'])},{x['bytes']}}},")
    h+=["};","inline constexpr EventSheetDef kEventSheets[]={"]
    for x in sheets:h.append(f"{{{q(x['name'])},{x['first_event']},{x['event_nodes']},{x['variables']}}},")
    h+=["};","inline constexpr CommandCount kConditionTypes[]={"]
    for i,n in sorted(cond.items()):h.append(f"{{{i},{n}}},")
    h+=["};","inline constexpr CommandCount kActionTypes[]={"]
    for i,n in sorted(acts.items()):h.append(f"{{{i},{n}}},")
    h+=["};","} // namespace minidayz::stage1",""]
    (out/"stage1_data.hpp").write_text("\n".join(h),encoding="utf-8")
    print(json.dumps({k:manifest[k] for k in ("objects","layers","instances","animations","image_frame_references","audio_resources","event_nodes","variables","groups")},indent=2))
if __name__=="__main__":main()
