#!/usr/bin/env python3
"""Extract the exact original Check_loot -> Global_respawn -> Trigger_spawn
category-15 branches. Fail closed if a guard, source value or action changes.
No guessed loot markers or item pools.
"""
import json, re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
project = json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-",
    (ROOT/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")))
out = ROOT/"include/minidayz/generated/original_timed_loot.hpp"

def children(e):
    return e[7] if len(e)>7 and isinstance(e[7],list) else []
def walk(events,parents=()):
    for e in events if isinstance(events,list) else []:
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,parents
        yield from walk(children(e),(*parents,e))
sheets=[s for s in project["project"][6] if isinstance(s,list) and len(s)>1]
events=[(e,parents) for s in sheets for e,parents in walk(s[1])]
def find_uid(uid):
    values=[e for e,_ in events if e[4]==uid]
    if len(values)!=1:raise SystemExit(f"Missing/duplicate original loot UID {uid}")
    return values[0]
def has_function(a,name):
    return isinstance(a,list) and len(a)>5 and a[:2]==[189,69] and isinstance(a[5],list) and len(a[5])>0 and a[5][0]==[1,[2,name]]
def has_condition(e,obj,command):
    return any(c[:2]==[obj,command] for c in e[5] if isinstance(c,list))
def checked(cond,obj,cmd,expected):
    return isinstance(cond,list) and cond[:2]==[obj,cmd] and cond[9]==expected
def p7(node):
    if not isinstance(node,list) or len(node)!=2 or node[0]!=7:
        raise SystemExit(f"Unexpected original expression wrapper {node}")
    return node[1]
def weighted(expr):
    if isinstance(expr,list) and len(expr)==2 and expr[0]==0 and isinstance(expr[1],int):
        return [expr[1]]
    if isinstance(expr,list) and len(expr)==3 and expr[:2]==[19,82]:
        values=expr[2]
        if isinstance(values,list) and values and all(isinstance(v,list) and len(v)==2 and v[0]==0 and isinstance(v[1],int) for v in values):
            return [v[1] for v in values]
    raise SystemExit(f"Nonliteral category-15 timed loot choice: {expr}")

timer = find_uid(2026232979865106)
if len(timer[5])!=1 or timer[5][0][:3]!=[270,107,"Timer"] or timer[5][0][9]!=[[1,[2,"Check_loot"]]]:
    raise SystemExit("Check_loot timer conditions changed")
if len(timer[6])!=1 or not has_function(timer[6][0],"Global_respawn"):
    raise SystemExit("Check_loot no longer directly calls Global_respawn")
global_root=find_uid(388188595587361)
if len(global_root[5])!=3 or not has_condition(global_root,189,40) or not has_condition(global_root,-1,51) or not has_condition(global_root,-1,46):
    raise SystemExit("Global_respawn layout/bridge guards changed")
if not any(checked(c,-1,51,[[7,[19,52]],[8,0],[7,[2,"Map"]]]) for c in global_root[5]):
    raise SystemExit("Global_respawn is not Map-only")
if not any(checked(c,-1,46,[[11,"onBridge"],[8,0],[7,[0,0]]]) for c in global_root[5]):
    raise SystemExit("Global_respawn onBridge guard changed")
timer_nodes=list(walk([global_root]))
timed_calls=[e for e,_ in timer_nodes if any(has_function(a,"Trigger_spawn") for a in e[6])]
if len(timed_calls)!=1:
    raise SystemExit(f"Expected exact one timed Trigger_spawn event, got {len(timed_calls)}")
timed_event=timed_calls[0]
if len(timed_event[5])!=2 or not any(checked(c,218,71,[[10,0],[8,0],[7,[0,15]]]) for c in timed_event[5]):
    raise SystemExit("Timed source marker type is no longer exactly 15")
radius=[c for c in timed_event[5] if c[:2]==[-1,89]]
if len(radius)!=1 or radius[0][9][0]!=[4,218] or radius[0][9][2]!=[8,4] or radius[0][9][3]!=[7,[0,1000]]:
    raise SystemExit("Timed source marker 1000-pixel distance guard changed")

trigger=find_uid(7952760861768009)
if not has_condition(trigger,189,40):
    raise SystemExit("Original Trigger_spawn function entry changed")
records=[]
for node,parents in walk([trigger]):
    path=(*parents,node)
    category=[c for e in path for c in e[5] if isinstance(c,list) and c[:2]==[218,71] and c[9]==[[10,0],[8,0],[7,[0,15]]]]
    if not category:continue
    calls=[a for a in node[6] if has_function(a,"Spawn_drop")]
    if not calls:continue
    if len(parents)<2:raise SystemExit("Missing original seed branch")
    branch=parents[1]
    if any(checked(c,-1,46,[[11,"SEED_pubg"],[8,0],[7,[0,1]]]) for c in branch[5]):
        seed=1
    elif has_condition(branch,-1,70):
        seed=0
    else:
        raise SystemExit(f"Unknown original timed loot seed branch {branch[4]}")
    for e in path:
        for c in e[5]:
            pair=c[:2]
            if pair not in ([189,40],[-1,46],[218,92],[218,71],[-1,70]):
                raise SystemExit(f"Unported category-15 loot condition {pair}: {c}")
            if pair==[-1,46] and not checked(c,-1,46,[[11,"SEED_pubg"],[8,0],[7,[0,1]]]):
                raise SystemExit(f"Unexpected category-15 source global guard {c}")
            if pair==[218,71] and not checked(c,218,71,[[10,0],[8,0],[7,[0,15]]]):
                raise SystemExit(f"Unexpected category-15 source marker guard {c}")
    for action in calls:
        params=action[5]
        if not isinstance(params,list) or len(params)!=2 or not isinstance(params[1],list) or params[1][0]!=13 or len(params[1])!=7:
            raise SystemExit("Unexpected Spawn_drop call signature")
        args=params[1][1:]
        items=weighted(p7(args[0]))
        for axis,ace in ((1,65),(2,66)):
            if args[axis] != [7,[19,80,[[20,218,ace,False,None]]]]:
                raise SystemExit(f"Unexpected original timed drop coordinate {args[axis]}")
        if args[3:]!=[[7,[0,-1]]]*3:
            raise SystemExit("Unexpected original timed Spawn_drop suffix")
        records.append((seed,node[4],action[3],items))
if len(records)!=2 or sorted(x[0] for x in records)!=[0,1]:
    raise SystemExit(f"Original timed category 15 must have one weighted branch per seed mode: {records}")
rows=[]
flat=[]
for seed,event,action,items in sorted(records):
    rows.append((seed,event,action,len(flat),len(items)))
    flat+=items

# Preserve exact root-authored marker instance variable 0 for native layout
# readers; the 15 editor palette markers are deliberately outside active Map.
map_layout=next(x for x in project["project"][5] if x[0]=="Map")
markers=[]
for layer in map_layout[6]:
    if not isinstance(layer,list) or len(layer)<15:continue
    for instance in layer[14] if isinstance(layer[14],list) else []:
        if not isinstance(instance,list) or len(instance)<4 or instance[1]!=218:continue
        marker_type=instance[3][0][0] if isinstance(instance[3],list) and instance[3] and isinstance(instance[3][0],list) and instance[3][0] else None
        if not isinstance(marker_type,int):
            raise SystemExit("Root Map marker lacks original variable-zero category")
        markers.append((instance[2],marker_type))
if len(markers)!=15:
    raise SystemExit(f"Expected 15 original Map loot editor markers, found {len(markers)}")

header=["#pragma once","#include <cstddef>","#include <cstdint>","namespace minidayz::items {",
    "struct OriginalTimedLootSourcePool { std::uint8_t seed_pubg; std::uint64_t event_uid, action_uid; std::uint16_t first, count; };",
    "struct OriginalMapLootMarker { std::uint64_t source_uid; std::uint16_t category; };",
    f"inline constexpr std::size_t kOriginalTimedLootPoolCount = {len(rows)};",
    f"inline constexpr std::size_t kOriginalTimedLootChoiceCount = {len(flat)};",
    "inline constexpr std::uint16_t kOriginalTimedLootChoices[] = {",
    "    "+", ".join(map(str,flat))+",","};",
    "inline constexpr OriginalTimedLootSourcePool kOriginalTimedLootPools[] = {"]
for seed,event,action,first,count in rows:
    header.append(f"    {{{seed}, {event}ULL, {action}ULL, {first}, {count}}},")
header+=["};","inline constexpr OriginalMapLootMarker kOriginalMapLootMarkers[] = {"]
for uid,category in sorted(markers):
    header.append(f"    {{{uid}ULL, {category}}},")
header+=["};","inline constexpr std::size_t kOriginalMapLootMarkerCount = 15;",
    "} // namespace minidayz::items",""]
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text("\n".join(header),encoding="utf-8")
print("SOURCE TIMED LOOT",len(records),"exact seed branches",len(flat),"weighted item choices","marker source UID",timed_event[4],"editor markers",len(markers))
