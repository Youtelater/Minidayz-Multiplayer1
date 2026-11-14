#!/usr/bin/env python3
"""Export original inventory-item behavior events without inventing gameplay data.

The trace preserves Construct condition/action records and source event UIDs for
all item use, pickup, inventory transfer, drop, and backpack checks. A later C++
translation must map command IDs and expressions to verified runtime behavior.
"""
from __future__ import annotations

import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
ITEMS = ROOT / "Minidayz-Multiplayer-main/l_eng_items.xml"
ITEM_TABLE = ROOT / "mp_itemtable.js"
OUTPUT = ROOT / "data/stage2/item_behavior_trace.json"

TARGETS = {
    3544291115846117: "Using inventory items",
    115345084350351: "Picking player items",
    1529638233252698: "Picking inventory items",
    594580094599696: "Droppin items",
    3439113896437199: "inventory_items",
    8381242860269222: "item_activate",
    3890676334551161: "item_activate2",
    240035455345341: "Check_space_backpack",
    6633395042052185: "Check_item_backpack",
}


def parts(event):
    if not isinstance(event, list) or not event or event[0] != 0:
        return None
    header = event[1] if len(event) > 1 else None
    name = header[1] if isinstance(header, list) and len(header) > 1 and isinstance(header[1], str) else ""
    uid = event[4] if len(event) > 4 and isinstance(event[4], int) else 0
    conditions = event[5] if len(event) > 5 and isinstance(event[5], list) else []
    actions = event[6] if len(event) > 6 and isinstance(event[6], list) else []
    children = event[7] if len(event) > 7 and isinstance(event[7], list) else []
    return uid, name, conditions, actions, children


def find_groups(entries, found, source_sheet):
    for event in entries if isinstance(entries, list) else []:
        p = parts(event)
        if p is None:
            continue
        uid, _, _, _, children = p
        if uid in TARGETS:
            if uid in found:
                raise SystemExit(f"Duplicate source event group UID: {uid}")
            found[uid] = (event, source_sheet)
        find_groups(children, found, source_sheet)


def flatten(event, parent_uid=0, depth=0):
    p = parts(event)
    if p is None:
        return
    uid, name, conditions, actions, children = p
    yield {
        "uid": uid,
        "parent_uid": parent_uid,
        "depth": depth,
        "name": name,
        "conditions": conditions,
        "actions": actions,
    }
    for child in children:
        yield from flatten(child, uid, depth + 1)


def main() -> None:
    for path in (PROJECT, ITEMS, ITEM_TABLE):
        if not path.is_file():
            raise SystemExit(f"Missing original source input: {path}")

    project = json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", PROJECT.read_text(encoding="utf-8")))
    sheets = project.get("project", [None] * 7)[6]
    if not isinstance(sheets, list):
        raise SystemExit("Original event sheets not found")

    # Backpack validation functions live on the original GUI sheet, while the
    # pickup/drop groups live on Game_events. Search every original event sheet
    # so the trace does not silently omit those source functions.
    found = {}
    for sheet in sheets:
        if not isinstance(sheet, list) or len(sheet) < 2 or not isinstance(sheet[0], str):
            continue
        find_groups(sheet[1], found, sheet[0])

    missing = set(TARGETS) - set(found)
    if missing:
        raise SystemExit(f"Missing original inventory event groups: {sorted(missing)}")

    xml_root = ET.parse(ITEMS).getroot()
    item_rows = []
    for item in xml_root.findall("item"):
        raw_id, name = item.findtext("id"), item.findtext("name")
        if raw_id is None or name is None:
            raise SystemExit("Original item localization row is missing id/name")
        item_rows.append({"id": int(raw_id.strip()), "name": name.strip()})
    if len(item_rows) != 405:
        raise SystemExit(f"Expected 405 original item names, found {len(item_rows)}")

    table = ITEM_TABLE.read_text(encoding="utf-8")
    table = re.sub(r"\\u([0-9a-fA-F]{4})",
                   lambda m: chr(int(m.group(1), 16)), table)
    marker = table.find("MDZItemTable")
    start = table.find("{", marker)
    end = table.find("}", start)
    if marker < 0 or start < 0 or end < 0:
        raise SystemExit("Original MDZItemTable not found")
    sprite_objects = {
        int(m.group(1)): json.loads(m.group(2))
        for m in re.finditer(r'(\d+)\s*:\s*("[^"]*")', table[start + 1:end])
    }

    groups = []
    for uid, label in TARGETS.items():
        event, source_sheet = found[uid]
        rows = list(flatten(event))
        groups.append({
            "source_uid": uid,
            "source_name": label,
            "source_sheet": source_sheet,
            "event_count": len(rows),
            "events": rows,
        })

    result = {
        "source": {
            "project": "data/stage1/construct_project_parsed.json",
            "item_names": "Minidayz-Multiplayer-main/l_eng_items.xml",
            "item_objects": "mp_itemtable.js",
        },
        "item_count": len(item_rows),
        "items": [
            {**item, "construct_object": sprite_objects.get(item["id"])}
            for item in item_rows
        ],
        "groups": groups,
        "interpretation": (
            "Raw original Construct conditions/actions are preserved. Numeric command "
            "IDs and expression parameters are intentionally not guessed or converted "
            "into gameplay effects by this exporter."
        ),
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(result, ensure_ascii=False, separators=(",", ":")),
                      encoding="utf-8")
    print(f"Exported {len(item_rows)} original items and "
          f"{sum(g['event_count'] for g in groups)} inventory behavior event records")


if __name__ == "__main__":
    main()
