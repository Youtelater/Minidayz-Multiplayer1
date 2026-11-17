#!/usr/bin/env python3
"""Extract all original Construct Craft_* functions and source result actions.

This is the authoritative source event index, NOT an invented inventory
recipe list. Building_mode and CreateObject cannot be treated as inventory
outputs. The native game may execute an entry only after original conditions,
materials and world placement have been reproduced.
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "data/stage1/construct_project_parsed.json"
OUT = ROOT / "include/minidayz/generated/source_craft_events.hpp"
KINDS = {
    "ground":"ground_item",
    "inventory":"inventory_item",
    "building":"building_mode",
    "world":"world_object",
}
EXPECTED = 19

def children(e):
    return e[7] if isinstance(e,list) and len(e)>7 and isinstance(e[7],list) else []

def walk(events):
    for e in events if isinstance(events,list) else []:
        if isinstance(e,list) and len(e)>6 and e[0]==0:
            yield e
            yield from walk(children(e))

def craft_function(e):
    for c in e[5]:
        if not isinstance(c,list) or len(c)<10 or c[:2]!=[292,71] or [10,0] not in c[9]:
            continue
        for arg in c[9]:
            if (isinstance(arg,list) and len(arg)==2 and arg[0]==7 and
                isinstance(arg[1],list) and len(arg[1])==2 and arg[1][0]==2):
                return arg[1][1]
    return None

def literal_int(expr):
    if isinstance(expr,list) and len(expr)==2 and expr[0]==7:
        n=expr[1]
        if isinstance(n,list) and len(n)==2 and n[0]==0 and isinstance(n[1],int):
            return n[1]
    return None

def action_output(a):
    if not isinstance(a,list) or len(a)<6:
        return None
    if a[:2]==[-1,94] and a[5] and isinstance(a[5][0],list) and a[5][0][0]==4:
        return ("world",a[5][0][1])
    if a[:2]!=[189,69] or len(a[5])<2 or not isinstance(a[5][0],list):
        return None
    target=a[5][0]
    if len(target)<2 or target[0]!=1 or not isinstance(target[1],list):
        return None
    name=target[1][1] if len(target[1])>1 and target[1][0]==2 else None
    args=a[5][1]
    if not isinstance(args,list) or len(args)<2 or args[0]!=13:
        return None
    if name=="Spawn_drop" and len(args)>1:
        number=literal_int(args[1])
        if number is not None:return ("ground",number)
    if name=="Check_space_inventory" and len(args)>2 and args[1]==[7,[0,-1]]:
        number=literal_int(args[2])
        if number is not None:return ("inventory",number)
    if name=="Building_mode":
        number=literal_int(args[1])
        if number is not None:return ("building",number)
    return None

def main():
    data=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
                           SOURCE.read_text(encoding="utf-8-sig")))
    found={}
    for sheet in data["project"][6]:
        if not isinstance(sheet,list) or len(sheet)<2:continue
        for e in walk(sheet[1]):
            name=craft_function(e)
            if isinstance(name,str) and name.startswith("Craft_"):
                if name in found:raise SystemExit(f"Ambiguous source function: {name}")
                found[name]=e
    if len(found)!=EXPECTED:
        raise SystemExit(f"Original crafting function count changed: {len(found)}")
    rows=[]
    for name,root in sorted(found.items()):
        outcomes=set()
        delays=set()
        for e in walk([root]):
            for action in e[6]:
                out=action_output(action)
                if out is not None:outcomes.add(out)
                if (isinstance(action,list) and len(action)>5 and action[:2]==[-1,54] and
                    isinstance(action[5],list) and len(action[5])==1 and
                    isinstance(action[5][0],list) and action[5][0][0]==0 and
                    isinstance(action[5][0][1],list) and len(action[5][0][1])==2 and
                    action[5][0][1][0]==0):
                    val=action[5][0][1][1]
                    if isinstance(val,(int,float)) and val>=1:
                        delays.add(float(val))
        if len(outcomes)!=1:
            raise SystemExit(f"Source crafting {name} has {sorted(outcomes)} outcomes")
        if len(delays)>1:
            raise SystemExit(f"Source crafting {name} has different long waits {delays}")
        kind,result=next(iter(outcomes))
        if not isinstance(result,int) or result<0 or result>65535:
            raise SystemExit(f"Unsupported craft result for {name}")
        rows.append((name,root[4],kind,result,next(iter(delays),0.0)))
        print(f"ORIGINAL CRAFT EVENT {name}: uid={root[4]} {kind}={result} wait={next(iter(delays),0.0)}")
    lines=[
        "#pragma once",
        "#include <cstddef>",
        "#include <cstdint>",
        "namespace minidayz::items {",
        "enum class SourceCraftOutputKind : std::uint8_t {ground_item, inventory_item, building_mode, world_object};",
        "struct SourceCraftEvent {",
        "    const char* function;",
        "    std::uint64_t source_uid;",
        "    SourceCraftOutputKind kind;",
        "    std::uint16_t result;",
        "    float source_wait_seconds;",
        "};",
        f"inline constexpr std::size_t kOriginalCraftFunctionCount = {len(rows)};",
        "inline constexpr SourceCraftEvent kOriginalCraftFunctions[] = {",
    ]
    for name,uid,kind,result,wait in rows:
        lines.append(f'    {{"{name}",{uid}ULL,SourceCraftOutputKind::{KINDS[kind]},{result},{wait:.2f}f}},')
    lines += ["};", "} // namespace minidayz::items", ""]
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text("\n".join(lines),encoding="utf-8")
if __name__=="__main__":main()
