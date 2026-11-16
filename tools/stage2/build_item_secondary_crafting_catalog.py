#!/usr/bin/env python3
"""Extract the original 3 Rags -> Rope secondary inventory action.

Original Construct only. The item_activate2 branch for item 26 requires
the SELECTED stack count >= 3, subtracts 3 from that stack, then calls the
original Check_space_inventory with result item ID 41.
"""
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path
from build_item_effect_catalog import children, find_uid, item_parameter

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "data/stage1/construct_project_parsed.json"
ITEMS = ROOT / "l_eng_items.xml"
OUT = ROOT / "include/minidayz/generated/item_secondary_crafting_catalog.hpp"

def main():
    names = {int(x.findtext("id")): (x.findtext("name") or "").strip()
             for x in ET.parse(ITEMS).getroot().findall("item")}
    if names.get(26) != "Rags" or names.get(41) != "Rope":
        raise SystemExit("Original Rags/Rope localization IDs changed")
    data = SRC.read_text(encoding="utf-8-sig")
    data = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", data)
    project = json.loads(data)
    sheets = project["project"][6]
    game = next((s for s in sheets if isinstance(s,list) and len(s)>1
                 and s[0]=="Game_events"), None)
    if not game:
        raise SystemExit("Original Game_events missing")
    group = find_uid(game[1],3890676334551161)
    if not group:
        raise SystemExit("Original item_activate2 missing")
    records = [event for event in children(group)
               if isinstance(event,list) and len(event)>7
               and item_parameter(event)==26]
    if len(records)!=1:
        raise SystemExit(f"Original Rags secondary event changed: {len(records)}")
    root=records[0]
    matches=[]
    for node in children(root):
        if not isinstance(node,list) or len(node)<7:
            continue
        conditions=node[5] if isinstance(node[5],list) else []
        has_count=any(isinstance(c,list) and len(c)>9 and c[0:2]==[189,112]
                      and c[9]==[[0,[0,1]],[8,5],[7,[0,3]]]
                      for c in conditions)
        if not has_count:
            continue
        actions=node[6] if isinstance(node[6],list) else []
        decrement=[a for a in actions if isinstance(a,list) and len(a)>5
                   and a[0:2]==[189,272] and a[5] ==
                   [[7,[5,[20,189,93,False,None,[[0,1]]],[0,3]]]]]
        output=[a for a in actions if isinstance(a,list) and len(a)>5
                and a[0:2]==[189,69] and a[5] ==
                [[1,[2,"Check_space_inventory"]],
                 [13,[7,[0,-1]],[7,[0,41]]]]]
        if len(decrement)!=1 or len(output)!=1:
            raise SystemExit(f"Original Rags craft expression changed: {node[4]}")
        matches.append((node[4],decrement[0][3],output[0][3]))
    if len(matches)!=1:
        raise SystemExit(f"Missing/unexpected Rags secondary craft actions: {matches}")
    event_uid,remove_uid,output_uid=matches[0]
    lines=[
        "#pragma once","#include <cstdint>",
        "namespace minidayz::items {",
        "inline constexpr std::uint16_t kSourceRagsToRopeInputId=26;",
        "inline constexpr std::uint16_t kSourceRagsToRopeRequiredCount=3;",
        "inline constexpr std::uint16_t kSourceRagsToRopeOutputId=41;",
        f"inline constexpr std::uint64_t kSourceRagsToRopeEventUid={event_uid}ULL;",
        f"inline constexpr std::uint64_t kSourceRagsToRopeDecrementUid={remove_uid}ULL;",
        f"inline constexpr std::uint64_t kSourceRagsToRopeOutputUid={output_uid}ULL;",
        "} // namespace minidayz::items",""]
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text("\n".join(lines),encoding="utf-8")
    print(f"Verified original Rags x3 -> Rope source UIDs {matches[0]}")

if __name__=="__main__":
    main()
