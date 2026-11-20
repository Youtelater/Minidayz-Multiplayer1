#!/usr/bin/env python3
"""Extract root-original Construct ground_enviroment SetTile scatter rules.

The original Game_events create_map_location function calls Tilemap(230)
Set tile at with floor(random(min,max)), choose(weighted IDs), and Repeat.
Keep the exact tile-choice multiplicities and source marker predicates.
Rules requiring unsupported coordinates/branch predicates are counted rather
than silently fabricated.
"""
from __future__ import annotations
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
PROJECT=ROOT/"data.deobfuscated.js"
OUT=ROOT/"include/minidayz/generated/terrain_scatter_source.hpp"

def events(nodes,parents=()):
    for event in nodes:
        if not isinstance(event,list) or not event or event[0]!=0:
            continue
        yield event,parents
        children=event[7] if len(event)>7 and isinstance(event[7],list) else []
        yield from events(children,parents+(event,))

def coords(node):
    try:
        expression=node[1]
        # The original forest-interior branch paints 20 cells at
        # 10 + floor(random(13)), in both axes. Keep its offset instead
        # of discarding that SetTile action as unsupported source art.
        if (expression[0]==4 and len(expression)==3 and
            expression[1][0]==0 and isinstance(expression[1][1],int)):
            nested=coords([0,expression[2]])
            if nested is not None:
                return (expression[1][1]+nested[0],
                        expression[1][1]+nested[1])
        if expression[0]!=19 or expression[1]!=80:
            return None
        random=expression[2][0]
        if random[0]!=19 or random[1]!=95:
            return None
        values=[arg[1] for arg in random[2] if arg[0]==0]
        if len(values)==1 and isinstance(values[0],int) and values[0]>0:
            return (0,values[0])
        if len(values)==2 and all(isinstance(n,int) for n in values) and values[1]>values[0]:
            return tuple(values)
    except (IndexError,TypeError,KeyError):
        return None
    return None

def choices(node):
    try:
        expression=node[1]
        if expression[0]!=19 or expression[1]!=82:
            return None
        values=[arg[1] for arg in expression[2] if arg[0]==0]
        if not values or not all(isinstance(n,int) and 0<=n<65535 for n in values):
            return None
        return values
    except (IndexError,TypeError,KeyError):
        return None

def main():
    project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",
        PROJECT.read_text(encoding="utf-8-sig")))["project"]
    rules=[]
    rejected=[]
    for sheet in project[6]:
        if sheet[0]!="Game_events":
            continue
        for event,parents in events(sheet[1]):
            for action in event[6] if len(event)>6 and isinstance(event[6],list) else []:
                if not isinstance(action,list) or len(action)<6 or action[0]!=230 or action[1]!=192:
                    continue
                ancestry=parents+(event,)
                conds=[condition for node in ancestry for condition in
                       (node[5] if len(node)>5 and isinstance(node[5],list) else [])
                       if isinstance(condition,list) and len(condition)>9]
                markers=[]
                for condition in conds:
                    if condition[0]==514 and condition[1]==71:
                        try:
                            value=condition[9][2][1][1]
                            if isinstance(value,int):
                                markers.append(value)
                        except (IndexError,TypeError):
                            pass
                func=any(condition[0]==189 and condition[1]==40 and
                         "create_map_location" in str(condition[9]) for condition in conds)
                if not func or not markers:
                    continue
                # Original CurrentLevel/perk_dao branches have a matching
                # Else. Keep the ordinary branch until perk state is wired.
                special=any(condition[0]==-1 and condition[1]==46 and
                            "perk_dao" in str(condition[9]) for condition in conds)
                if special:
                    rejected.append((action[3],"special perk branch"))
                    continue
                repeat=1
                for condition in conds:
                    if condition[0]==-1 and condition[1]==123:
                        try:
                            repeat=int(condition[9][0][1][1])
                        except (IndexError,TypeError,ValueError):
                            repeat=0
                x=coords(action[5][0]) if len(action[5])>0 else None
                y=coords(action[5][1]) if len(action[5])>1 else None
                tiles=choices(action[5][2]) if len(action[5])>2 else None
                if not x or not y or not tiles or not 0<repeat<=400:
                    rejected.append((action[3],"unsupported source expression"))
                    continue
                for marker in sorted(set(markers)):
                    rules.append((marker,repeat,*x,*y,tiles,action[3]))
    if len(rules)<45:
        raise SystemExit(f"Only {len(rules)} location scatter rules extracted; source changed")
    palette=[]
    entries=[]
    for marker,repeat,xlo,xhi,ylo,yhi,tiles,uid in rules:
        offset=len(palette)
        palette.extend(tiles)
        entries.append((marker,repeat,xlo,xhi,ylo,yhi,offset,len(tiles),uid))
    OUT.parent.mkdir(parents=True,exist_ok=True)
    lines=["#pragma once","#include <cstddef>","#include <cstdint>",
           "namespace minidayz::stage1 {",
           "// Original source object 230, 30px cells, created at hotspot minus 500px.",
           "struct TerrainScatterRule {",
           " std::uint16_t marker, repeat, x_min, x_max, y_min, y_max;",
           " std::uint32_t tile_offset; std::uint16_t tile_count;",
           " std::uint64_t source_action_uid;",
           "};",
           "inline constexpr std::uint16_t kTerrainScatterChoices[]={"]
    lines.extend(" "+str(v)+"," for v in palette)
    lines.extend(["};","inline constexpr TerrainScatterRule kTerrainScatterRules[]={"])
    for entry in entries:
        lines.append(" {"+",".join(str(v) for v in entry)+"},")
    lines.extend(["};",
        "inline constexpr std::size_t kTerrainScatterRuleCount=sizeof(kTerrainScatterRules)/sizeof(kTerrainScatterRules[0]);",
        "inline constexpr std::size_t kTerrainScatterChoiceCount=sizeof(kTerrainScatterChoices)/sizeof(kTerrainScatterChoices[0]);",
        "}"])
    OUT.write_text("\n".join(lines)+"\n",encoding="utf-8")
    print(f"Original environment scatter: {len(entries)} marker rules, "
          f"{len(palette)} weighted source frames, "
          f"{len(rejected)} conditional/unsupported actions withheld")
    if rejected:
        print("Withheld source action UIDs:",", ".join(str(uid) for uid,_ in rejected))

if __name__=="__main__":
    main()
