#!/usr/bin/env python3
"""Extract EXACT original survivor hand overlays from root Construct events.

Game_events Start events assign PlayerBase193 variable0 = skin.SpriteUID
and variable1 = matching hands.SpriteUID for Player_skin_type 1..21.
These are distinct source Sprite animations on original Map layer 34.
Never invent skin colors or draw a generic replacement arm graphic.
"""
from pathlib import Path
import json
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode

project=decode((ROOT/"data.deobfuscated.js").read_bytes())
objects=project[3]
events=dict(project[6])["Game_events"]
OUT=ROOT/"include/minidayz/generated/player_hands_source.hpp"
DIRECTIONS=("down","up","right","left")

def walk(entries):
    for event in entries if isinstance(entries,list) else []:
        if not isinstance(event,list) or len(event)<7 or event[0]!=0:
            continue
        yield event
        if len(event)>7 and isinstance(event[7],list):
            yield from walk(event[7])

def original_object_uid(action,slot):
    if not (isinstance(action,list) and len(action)>5 and
            action[:2]==[193,88] and len(action[5])==2 and
            action[5][0]==[10,slot]):
        return None
    value=action[5][1]
    if not (isinstance(value,list) and len(value)==2 and
            value[0]==7 and isinstance(value[1],list) and
            len(value[1])==5 and value[1][0]==20 and
            value[1][2]==151):
        raise SystemExit(f"PlayerBase193 hand/skin source UID AST changed: {action}")
    return int(value[1][1])

def source_skin_type(event):
    matches=[]
    for cond in event[5] if isinstance(event[5],list) else []:
        if not (isinstance(cond,list) and len(cond)>9 and
                cond[:2]==[-1,46] and isinstance(cond[9],list) and
                cond[9][:2]==[[11,"Player_skin_type"],[8,0]]):
            continue
        rhs=cond[9][2]
        if not (rhs[0]==7 and rhs[1][0]==0 and
                isinstance(rhs[1][1],int)):
            raise SystemExit("Unexpected original Player_skin_type expression")
        matches.append(rhs[1][1])
    if not matches:return None
    if len(matches)!=1:raise SystemExit("Ambiguous source skin event guard")
    return matches[0]

source_hands={}
source_types=set()
for event in walk(events):
    skin_type=source_skin_type(event)
    if skin_type is None:continue
    skin=[v for a in event[6] if
          (v:=original_object_uid(a,0)) is not None]
    hands=[v for a in event[6] if
           (v:=original_object_uid(a,1)) is not None]
    if not skin and not hands:continue
    if len(skin)!=1 or len(hands)!=1:
        raise SystemExit(f"Original skin type {skin_type}: missing linked hands")
    if skin_type in source_types:
        raise SystemExit(f"Duplicate Player_skin_type {skin_type}")
    source_types.add(skin_type)
    source_hands[skin[0]]=(hands[0],skin_type,event[4])

if source_types!=set(range(1,22)) or len(source_hands)!=21:
    raise SystemExit(f"Original PlayerBase skin/hand records missing: {source_types}")
if source_hands[679][:2]!=(682,2) or source_hands[744][:2]!=(681,8) or \
   source_hands[1262][:2]!=(1263,21):
    raise SystemExit("Original survivor skin/hand family source has changed")

for skin,(hands,kind,event_uid) in source_hands.items():
    skin_animations=objects[skin][7]
    hand_animations=objects[hands][7]
    def by_name(animations):
        return {str(a[0]).lower():a for a in animations
                if isinstance(a,list) and len(a)>7 and isinstance(a[7],list)}
    actor=by_name(skin_animations)
    limb=by_name(hand_animations)
    for name in (f"{verb}_{direction}" for verb in ("idle","run")
                 for direction in DIRECTIONS):
        if not actor.get(name) or not actor[name][7] or \
           not limb.get(name) or not limb[name][7]:
            raise SystemExit(
                f"Skin {skin} / hands {hands} missing authored {name}")
    # The source hand object is a real hand animation, not an inventory item.
    first=limb["idle_down"][7][0]
    if not (isinstance(first,list) and first and isinstance(first[0],str)
            and Path(first[0]).name.startswith("player_hands_")):
        raise SystemExit(f"Source hands {hands} does not use player_hands art")
    if kind<1 or kind>21 or event_uid<=0:
        raise SystemExit("Malformed original survivor skin source event")

rows=sorted((skin,*entry) for skin,entry in source_hands.items())
lines=["#pragma once","#include <cstdint>","#include <cstddef>",
       "namespace minidayz::stage1 {",
       "// Root Start events PlayerBase193 variables0/1: original matching skin and hands.",
       "struct OriginalPlayerHandArt {",
       " std::uint16_t skin, hands;",
       " std::uint8_t source_skin_type;",
       " std::uint64_t source_event_uid; };",
       f"inline constexpr std::size_t kOriginalPlayerHandArtCount={len(rows)};",
       "inline constexpr OriginalPlayerHandArt kOriginalPlayerHandArt[]={"]
lines.extend(f" {{{skin},{hand},{kind},{uid}ULL}}," for
             skin,hand,kind,uid in rows)
lines += ["};",
"""inline std::uint16_t source_player_hands_object(std::uint16_t skin) noexcept {
    for(const auto& entry:kOriginalPlayerHandArt)
        if(entry.skin==skin)return entry.hands;
    return 0;
}""",
"} // namespace minidayz::stage1",""]
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text("\n".join(lines),encoding="utf-8")
print("PASS ORIGINAL PLAYER HANDS",json.dumps({
    "source_skin_types":len(source_types),
    "linked_original_hand_sprites":len(rows),
    "biker_original_hand_sprite":source_hands[744][0],
    "default_original_hand_sprite":source_hands[679][0],
},sort_keys=True))
