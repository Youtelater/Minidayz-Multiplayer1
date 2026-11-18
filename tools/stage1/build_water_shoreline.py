#!/usr/bin/env python3
"""Extract the original water (208) shoreline SetTile neighbor predicates.

Original Game_events compares water tile IDs at eight surrounding cells,
then selects a source frame in tilemap_water.png. Preserve the exact source
condition masks; generated overlays are applied only to EMPTY authored cells
next to source water, without rewriting original RLE artwork.
"""
import json
import re
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]
PROJECT=ROOT/"data/stage1/construct_project_parsed.json"
OUTPUT=ROOT/"include/minidayz/generated/water_shore_source.hpp"

def walk(events,ancestors=()):
    for event in events:
        if not isinstance(event,list) or not event or event[0]!=0:continue
        yield event,ancestors
        yield from walk(event[7] if len(event)>7 and isinstance(event[7],list) else [],
                        ancestors+(event,))

def offset(expr,axis):
    if expr==[23,axis]:return 0
    if isinstance(expr,list) and len(expr)==3 and expr[1]==[23,axis] and expr[2]==[0,1]:
        if expr[0]==5:return 1
        if expr[0]==4:return -1
    raise ValueError(f"Unexpected original water neighbor coordinate {expr}")

def main():
    project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
        PROJECT.read_text(encoding="utf-8-sig")))["project"]
    rules=[]
    for sheet in project[6]:
        if sheet[0]!="Game_events":continue
        for event,ancestors in walk(sheet[1]):
            for action in event[6] if len(event)>6 and isinstance(event[6],list) else []:
                if not isinstance(action,list) or len(action)<6 or action[0]!=208 or action[1]!=192:
                    continue
                conds=[c for node in ancestors+(event,)
                       for c in (node[5] if len(node)>5 and isinstance(node[5],list) else [])
                       if isinstance(c,list) and len(c)>9 and c[0]==208 and c[1]==205]
                frame=action[5][2][1][1]
                if not isinstance(frame,int):raise SystemExit("Unexpected water SetTile frame expression")
                checks=[]
                for condition in conds:
                    args=condition[9]
                    dx=offset(args[0][1],"x")
                    dy=offset(args[1][1],"y")
                    compare=args[2][1]
                    target=args[3][1][1]
                    if compare not in (0,1) or target!=4:
                        raise SystemExit("Original water shoreline comparison changed")
                    checks.append((dx,dy,compare==0))
                if not 7<=len(checks)<=8:
                    raise SystemExit("Unrecognized source water shoreline predicate")
                rules.append((frame,action[3],checks))
    if len(rules)!=12:
        raise SystemExit(f"Expected twelve original shoreline SetTile branches; found {len(rules)}")
    original=ROOT/"assets/gfx/tilemap_water.png"
    with Image.open(original) as art:
        count=(art.width//60)*(art.height//60)
    if any(frame<0 or frame>=count for frame,_,_ in rules):
        raise SystemExit("Original shoreline frame not in source PNG")
    checks_flat=[]
    entries=[]
    for frame,uid,checks in rules:
        entries.append((frame,len(checks_flat),len(checks),uid))
        checks_flat.extend(checks)
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    lines=["#pragma once","#include <cstdint>","#include <cstddef>",
           "namespace minidayz::stage1 {",
           "struct WaterShoreCheck {std::int8_t dx,dy; bool requires_water;};",
           "struct WaterShoreRule {std::uint16_t frame; std::uint16_t first,count; std::uint64_t uid;};",
           "inline constexpr WaterShoreCheck kWaterShoreChecks[]={"]
    lines.extend(f" {{{dx},{dy},{str(equal).lower()}}}," for dx,dy,equal in checks_flat)
    lines.append("};")
    lines.append("inline constexpr WaterShoreRule kWaterShoreRules[]={")
    lines.extend(" {"+",".join(str(x) for x in entry)+"}," for entry in entries)
    lines.extend(["};","inline constexpr std::size_t kWaterShoreRuleCount=sizeof(kWaterShoreRules)/sizeof(kWaterShoreRules[0]);","}"])
    OUTPUT.write_text("\n".join(lines)+"\n",encoding="utf-8")
    print(f"Extracted {len(rules)} source shoreline rules and {len(checks_flat)} exact water-neighbor predicates")

if __name__=="__main__":
    main()
