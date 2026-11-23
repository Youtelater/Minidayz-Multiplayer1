#!/usr/bin/env python3
"""Build the exact original survivor corpse skin lookup from root Construct.

Source Game_events Start events assigns Player_skin_type -> PlayerBase193 var0
(sprite family 1064) and death events spawn original Sprite813 on the
items_on_ground layer, choosing its 'id_' + skin instance variable 3 animation.
Only the mappings whose ORIGINAL 813 animation exists are emitted. Missing
source variants are tracked, never guessed or substituted.
"""
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode

HEADER=ROOT/"include/minidayz/generated/source_dead_skin_art.hpp"
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
game=dict(p[6])["Game_events"]
original_corpse=p[3][813][7]
corpse_anims={str(a[0]):a for a in original_corpse if isinstance(a,list) and len(a)>7}
if not corpse_anims:raise SystemExit("Original Sprite813 corpse animations missing")

def walk(nodes):
    for e in nodes if isinstance(nodes,list) else ():
        if not isinstance(e,list) or len(e)<7:continue
        if e[0]==0:
            yield e
        if len(e)>7:
            yield from walk(e[7])

mappings={}
unmapped={}
for e in walk(game):
    guards=[c for c in e[5] if isinstance(c,list) and len(c)>9 and
            c[0:2]==[-1,46] and c[9][0:2]==[[11,"Player_skin_type"],[8,0]]]
    if len(guards)!=1:continue
    g=guards[0]
    if g[9][2][0]!=7 or g[9][2][1][0]!=0:continue
    selection=g[9][2][1][1]
    actors=[a for a in e[6] if isinstance(a,list) and len(a)>5 and
            a[0:2]==[193,88] and a[5][0]==[10,0] and
            a[5][1][0]==7 and a[5][1][1][0]==20]
    if not actors:continue
    if len(actors)!=1:raise SystemExit(f"Ambiguous original skin {selection}")
    skin=actors[0][5][1][1][1]
    name=f"id_{selection}"
    a=corpse_anims.get(name)
    if a is None:
        unmapped[selection]=skin
        continue
    if not (a[2] is False and a[3]==1 and len(a[7])==6):
        raise SystemExit(f"Original corpse animation changed: {name}")
    if skin in mappings and mappings[skin]!=name:
        raise SystemExit(f"Original source skin {skin} has two corpse mappings")
    mappings[skin]=name

if len(mappings)!=20 or 679 not in mappings or mappings[679]!="id_2":
    raise SystemExit(f"Original playable corpse coverage changed: {mappings}")
if unmapped!={21:1262}:
    raise SystemExit(f"Unverified corpse variants changed: {unmapped}")
lines=[
    "#pragma once","#include <cstddef>","#include <cstdint>",
    "namespace minidayz::stage1 {",
    "// Root-original Sprite813/id_N source animation and 193 skin assignment.",
    "// Source Player_skin_type21 (object1262) has no authored 813/id_21.",
    "struct SourceDeadSkinArt {std::uint16_t source_skin;const char* animation;};",
    f"inline constexpr std::size_t kSourceDeadSkinCount={len(mappings)};",
    "inline constexpr SourceDeadSkinArt kSourceDeadSkins[]={"
]
for skin,name in sorted(mappings.items()):
    lines.append(f'{{{skin},"{name}"}},')
lines+=["};",
"""inline const char* source_dead_skin_animation(std::uint16_t source_skin) noexcept {
    for(const auto& entry:kSourceDeadSkins)
        if(entry.source_skin==source_skin)return entry.animation;
    return nullptr;
}""",
"} // namespace minidayz::stage1",""]
HEADER.parent.mkdir(parents=True,exist_ok=True)
HEADER.write_text("\n".join(lines),encoding="utf-8")
print(f"PASS: root Sprite813 death frames for {len(mappings)} skins; missing source variants: {unmapped}")
