#!/usr/bin/env python3
"""Extract the original backpack grid size from the Construct Items_update events."""
from __future__ import annotations
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
OUTPUT = ROOT / "include/minidayz/generated/backpack_capacity_catalog.hpp"
ITEMS = ROOT / "l_eng_items.xml"
UI = ROOT / "l_eng_ui.xml"
EXPECTED_SLOTS = list(range(1, 12))

def children(event):
    return event[7] if isinstance(event, list) and len(event) > 7 and isinstance(event[7], list) else []

def events(entries):
    for event in entries if isinstance(entries, list) else []:
        if not isinstance(event, list) or not event or event[0] != 0:
            continue
        yield event
        yield from events(children(event))

def function_name(event):
    conditions = event[5] if len(event) > 5 and isinstance(event[5], list) else []
    for condition in conditions:
        if not isinstance(condition, list) or len(condition) < 10:
            continue
        if condition[0] == 189 and condition[1] == 40:
            params = condition[9]
            if isinstance(params, list) and params and isinstance(params[0], list):
                value = params[0]
                if len(value) > 1 and isinstance(value[1], list) and len(value[1]) > 1:
                    return value[1][1]
    return None

def main() -> None:
    for source_file in (PROJECT, ITEMS, UI):
        if not source_file.is_file():
            raise SystemExit(f"Missing root-level original source: {source_file}")
    source = PROJECT.read_text(encoding="utf-8-sig")
    source = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", source)
    project = json.loads(source)
    sheets = project["project"][6]
    matches = [event for sheet in sheets for event in events(sheet[1])
               if function_name(event) == "Items_update" and len(event[5]) == 1]
    if len(matches) != 1:
        raise SystemExit(f"Expected one original Items_update function, found {len(matches)}")
    item_update = matches[0]
    backpack_indices = []
    for event in events(children(item_update)):
        actions = event[6] if len(event) > 6 and isinstance(event[6], list) else []
        for action in actions:
            if not isinstance(action, list) or len(action) < 6 or action[0] != 189 or action[1] != 69:
                continue
            params = action[5]
            if not isinstance(params, list) or len(params) < 2:
                continue
            call = params[0]
            if not (isinstance(call, list) and len(call) > 1 and
                    isinstance(call[1], list) and len(call[1]) > 1 and call[1][1] == "Drop_slot"):
                continue
            args = params[1]
            if not isinstance(args, list) or len(args) < 3:
                continue
            index_expr, category_expr = args[1], args[2]
            if not (isinstance(index_expr, list) and len(index_expr) > 1 and
                    isinstance(index_expr[1], list) and len(index_expr[1]) == 2 and
                    index_expr[1][0] == 0 and isinstance(index_expr[1][1], int)):
                continue
            if not (isinstance(category_expr, list) and len(category_expr) > 1 and
                    isinstance(category_expr[1], list) and len(category_expr[1]) == 2 and
                    category_expr[1][0] == 2 and category_expr[1][1] == "Backpack"):
                continue
            backpack_indices.append(index_expr[1][1])
    actual = sorted(backpack_indices)
    if actual != EXPECTED_SLOTS:
        raise SystemExit(f"Original backpack grid changed: expected {EXPECTED_SLOTS}, found {actual}")
    # The UI exposes up to 11 storage slots, but original tutorial text
    # explicitly specifies smaller capacities for two improvised packs.
    names = {}
    for item in ET.parse(ITEMS).getroot().findall("item"):
        name = (item.findtext("name") or "").strip()
        if name:
            names.setdefault(name, []).append(int(item.findtext("id")))
    texts = [(e.findtext("name") or "").strip()
             for e in ET.parse(UI).getroot().findall("text")]
    known = [
        ("Improvised Sling Bag",
         r"Rope\s*\+\s*Burlap sack\s*=\s*improvised bag\s*\((\d+) slots\)"),
        ("Improvised Backpack",
         r"Improvised bag\s*\+\s*3 wooden sticks\s*=\s*improvised backpack\s*\((\d+) slots\)"),
    ]
    pack_rows = []
    for label, pattern in known:
        ids = names.get(label, [])
        slots = [int(match.group(1)) for phrase in texts
                 if (match := re.fullmatch(pattern, phrase, flags=re.IGNORECASE))]
        if len(ids) != 1 or len(slots) != 1 or not 0 < slots[0] <= len(actual):
            raise SystemExit(f"Source backpack capacity missing or ambiguous: {label}")
        pack_rows.append((ids[0], slots[0]))
    # The root original l_eng_new.xml has a separate description ID space
    # for several backpacks (e.g. Taloon=301 versus inventory item=450).
    # Match its adjacent original <!-- item name --> comment, NOT the
    # description ID, to avoid assigning a helmet's description to a pack.
    # The improvised pair has conflicting guide/description values: preserve
    # the already-validated original tutorial capacities (+3/+5).
    descriptions = ROOT / "l_eng_new.xml"
    if not descriptions.is_file():
        raise SystemExit("Missing original root item descriptions")
    desc_text = descriptions.read_text(encoding="utf-8-sig")
    source_descriptions = {}
    for match in re.finditer(
        r"<!--\s*([^<>]+?)\s*-->\s*<text>\s*<id>\s*(\d+)\s*</id>\s*"
        r"<name>(.*?)</name>\s*</text>", desc_text, flags=re.DOTALL):
        label = match.group(1).strip()
        body = match.group(3)
        slot_match = re.search(r"(?<!Exclusive )Slots:\s*\+(\d+)", body)
        if not slot_match:
            continue
        exclusive_match = re.search(r"Exclusive Slots:\s*\+(\d+)", body)
        entry = (int(slot_match.group(1)),
                 int(exclusive_match.group(1)) if exclusive_match else 0)
        source_descriptions.setdefault(label, set()).add(entry)
    backpack_ids = (450, 451, 452, 453, 454, 455, 457, 458, 459, 460,
                    462, 752, 753, 754)
    original_by_id = {int(item.findtext("id")):
                      (item.findtext("name") or "").strip()
                      for item in ET.parse(ITEMS).getroot().findall("item")}
    rows = []
    for item_id in backpack_ids:
        name = original_by_id.get(item_id)
        matches = source_descriptions.get(name, set())
        if not name or len(matches) != 1:
            raise SystemExit(f"Ambiguous/missing original pack description: "
                             f"{item_id} {name!r}: {matches}")
        extra, exclusive = next(iter(matches))
        if item_id == pack_rows[0][0]:
            extra = pack_rows[0][1]
        elif item_id == pack_rows[1][0]:
            extra = pack_rows[1][1]
        if extra < 1 or extra > len(actual) or exclusive > len(actual):
            raise SystemExit(f"Invalid source backpack slots: {item_id} "
                             f"{extra} exclusive={exclusive}")
        rows.append((item_id, extra, exclusive))
    rows.sort()
    lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::items {",
        f"inline constexpr std::size_t kSourceBackpackStorageSlotCount = {len(actual)};",
        f"inline constexpr std::uint16_t kSourceImprovisedSlingBagItemId = {pack_rows[0][0]};",
        f"inline constexpr std::size_t kSourceImprovisedSlingBagSlots = {pack_rows[0][1]};",
        f"inline constexpr std::uint16_t kSourceImprovisedBackpackItemId = {pack_rows[1][0]};",
        f"inline constexpr std::size_t kSourceImprovisedBackpackSlots = {pack_rows[1][1]};",
        "struct SourceBackpackCapacityRow { std::uint16_t item_id; std::uint8_t extra_slots; std::uint8_t exclusive_slots; };",
        f"inline constexpr std::size_t kSourceBackpackCapacityRowCount = {len(rows)};",
        "inline constexpr SourceBackpackCapacityRow kSourceBackpackCapacityRows[] = {",
    ]
    for item_id, extra, exclusive in rows:
        lines.append(f"    {{{item_id}, {extra}, {exclusive}}},")
    lines += [
        "};",
        "} // namespace minidayz::items", ""
    ]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Extracted {len(actual)} original backpack UI slots, "
          f"{len(rows)} item-specific capacities: {rows}")

if __name__ == "__main__":
    main()
