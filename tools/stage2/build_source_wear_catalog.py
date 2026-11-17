#!/usr/bin/env python3
"""Compile exact original item-condition action UIDs with literal wear deltas.

Never infer the triggering gameplay event from an item name. This catalog
contains action identity/amount only. Dispatchers must verify all source
event guards before applying any write. Random, negative and dynamic
expressions are intentionally excluded until their semantics are ported.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INPUT = ROOT / "data/stage2/original_item_wear_actions.json"
OUTPUT = ROOT / "include/minidayz/generated/source_wear_catalog.hpp"
GROUPS = {
    "firearm": 6904772320561696,
    "alternate": 337954396197555,
    "melee": 3528035038609865,
    "damage": 811368909310732,
    "player_damage": 9520121190252054,
    "obstacle": 5836740205504893,
}

def hundredths(value):
    if not isinstance(value,(float,int)) or isinstance(value,bool):
        return None
    scaled = value*100
    if value < 0 or abs(scaled-round(scaled)) > 0.00001 or scaled>10000:
        return None
    return int(round(scaled))

def parse_write(action):
    if len(action)<6 or action[0]!=1030 or action[1] not in (87,88):
        return None
    args=action[5]
    if not isinstance(args,list) or len(args)!=2 or args[0]!=[10,6]:
        return None
    expr=args[1]
    if not isinstance(expr,list) or len(expr)!=2 or expr[0]!=7:
        return None
    value=expr[1]
    if not isinstance(value,list):
        return None
    # The source uses ?-expressions keyed to perk_wxnj. The source condition
    # and BOTH branches must match exactly, otherwise leave untranslated.
    if (len(value)==4 and value[0]==18 and value[1]==
          [12,[23,"perk_wxnj"],[0,1]] and value[2]==[0,0] and
          isinstance(value[3],list) and value[3][0]==0):
        units=hundredths(value[3][1])
        if units is not None and action[1]==87:
            return units,1
    # Literal zero writes must NOT be mapped to fractional subtraction
    # without the original opcode semantics. Leave them unsupported.
    return None

def main():
    if not INPUT.is_file():
        raise SystemExit("Missing original wear source trace")
    data=json.loads(INPUT.read_text(encoding="utf-8"))
    rows=[]
    unsupported=0
    for entry in data:
        group=GROUPS.get(entry["name"])
        if not group: raise SystemExit("Unknown wear group")
        for action in entry["writes"]:
            parsed=parse_write(action)
            if parsed is None:
                unsupported+=1
                continue
            delta,perk=parsed
            rows.append((group,int(entry["uid"]),int(action[3]),delta,perk))
    # Construct exports reuse some event/action UIDs in cloned ACE branches.
    # Do not collapse them into an ambiguous runtime dispatch key: leave
    # every such occurrence untranslated until original ancestor identity
    # is passed with the event.
    occurrences={}
    for row in rows:
        key=row[:3]
        occurrences[key]=occurrences.get(key,0)+1
    ambiguous=sum(1 for row in rows if occurrences[row[:3]]>1)
    rows=[row for row in rows if occurrences[row[:3]]==1]
    unsupported+=ambiguous
    rows.sort()
    if len(rows)<80 or unsupported<5:
        raise SystemExit(f"Unexpected original wear extraction {len(rows)} supported, {unsupported} unresolved")
    lines=[
        "#pragma once",
        "#include <cstdint>",
        "#include <cstddef>",
        "namespace minidayz::items {",
        "struct SourceWearAction {",
        "    std::uint64_t group_uid;",
        "    std::uint64_t event_uid;",
        "    std::uint64_t action_uid;",
        "    std::uint16_t loss_hundredths;",
        "    bool cancel_when_wxnj_active;",
        "};",
        f"inline constexpr std::size_t kSourceWearActionCount = {len(rows)};",
        f"inline constexpr std::size_t kSourceWearUntranslatedCount = {unsupported};",
        "inline constexpr SourceWearAction kSourceWearActions[] = {",
    ]
    for group,event,action,delta,perk in rows:
        lines.append(f"    {{{group}ULL,{event}ULL,{action}ULL,{delta},{'true' if perk else 'false'}}},")
    lines += ["};", "} // namespace minidayz::items", ""]
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    OUTPUT.write_text("\n".join(lines),encoding="utf-8")
    print(f"Exact original wear actions with verified literal delta: {len(rows)}")
    print(f"Complex/untranslated source expressions: {unsupported}")
if __name__=="__main__":main()
