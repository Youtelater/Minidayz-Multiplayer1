#!/usr/bin/env python3
"""Extract original English MiniDAYZ achievements and character unlock text.

Authoritative single-player text is l_eng_ui.xml in the repository ROOT;
these are display records, not guesses about completion/progression state.
"""
from pathlib import Path
import json
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[2]
OUTPUT=ROOT/"include/minidayz/generated/menu_source_catalog.hpp"

def load():
    root=ET.parse(ROOT/"l_eng_ui.xml").getroot()
    strings={}
    for node in root.findall("text"):
        id_node=node.find("id")
        name_node=node.find("name")
        if id_node is not None and name_node is not None:
            key=int(id_node.text)
            if key in strings:
                # The supplied root XML reuses IDs 451+ between old
                # tutorial prompts and newer UI actions. These IDs are
                # unrelated to the original 71..79/101..183 menu text.
                if 71<=key<=183:
                    raise SystemExit(f"Conflicting original menu localization ID: {key}")
                continue
            strings[key]="".join(name_node.itertext()).strip()
    return strings

def build():
    strings=load()
    achievements=[]
    for n in range(33):
        identifier=101+n
        description_id=151+n
        if identifier not in strings or description_id not in strings:
            raise SystemExit(f"Missing original achievement localization {identifier}/{description_id}")
        achievements.append((identifier,strings[identifier],strings[description_id]))
    unlocks=[]
    for identifier in list(range(71,80))+[140,141,142]:
        if identifier not in strings:
            raise SystemExit(f"Missing original locked-character localization {identifier}")
        complete=strings[identifier]
        head=complete.splitlines()[0]
        unlocks.append((identifier,head,complete))
    lines=[
        "#pragma once","#include <cstddef>","#include <cstdint>",
        "namespace minidayz::menu {",
        "struct SourceMenuRecord { std::uint16_t localization_id; const char* name; const char* description; };",
        "// From root l_eng_ui.xml, IDs 101..133 / 151..183.",
        f"inline constexpr std::size_t kSourceAchievementCount={len(achievements)};",
        "inline constexpr SourceMenuRecord kSourceAchievements[] = {"
    ]
    for i,n,d in achievements:
        lines.append(f"  {{{i},{json.dumps(n,ensure_ascii=False)},{json.dumps(d,ensure_ascii=False)}}},")
    weapon_ids=[392,393,394,416,418,420]
    weapons=[]
    for identifier in weapon_ids:
        if identifier not in strings:
            raise SystemExit(f"Missing original weapon unlock localization {identifier}")
        complete=strings[identifier]
        weapons.append((identifier,complete.splitlines()[0],complete))
    lines += [
        "};",
        "// From root l_eng_ui.xml, survivor unlock IDs 71..79, 140..142.",
        f"inline constexpr std::size_t kSourceUnlockCount={len(unlocks)};",
        "inline constexpr SourceMenuRecord kSourceUnlocks[] = {"
    ]
    for i,n,d in unlocks:
        lines.append(f"  {{{i},{json.dumps(n,ensure_ascii=False)},{json.dumps(d,ensure_ascii=False)}}},")
    lines += [
        "};",
        "// From root l_eng_ui.xml, original weapon unlock requirement IDs.",
        f"inline constexpr std::size_t kSourceWeaponUnlockCount={len(weapons)};",
        "inline constexpr SourceMenuRecord kSourceWeaponUnlocks[] = {",
    ]
    for i,n,d in weapons:
        lines.append(f"  {{{i},{json.dumps(n,ensure_ascii=False)},{json.dumps(d,ensure_ascii=False)}}},")
    lines += ["};","} // namespace minidayz::menu",""]
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    OUTPUT.write_text("\n".join(lines),encoding="utf-8")
    print(f"PASS: source menu records {len(achievements)} achievements, {len(unlocks)} character unlocks and {len(weapons)} weapon unlocks")

if __name__=="__main__":
    build()
