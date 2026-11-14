#!/usr/bin/env python3
"""Extract original equipment and default-condition data from Construct map instances."""
from __future__ import annotations
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
XML = ROOT / "l_eng_items.xml"
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
OUTPUT = ROOT / "include/minidayz/generated/item_equipment_catalog.hpp"
CONDITION_OUTPUT = ROOT / "include/minidayz/generated/item_condition_catalog.hpp"
REPAIR_OUTPUT = ROOT / "include/minidayz/generated/item_outerwear_repair_catalog.hpp"
REPAIR_EVENTS = {
    5373788036138533: ("outerwear_tape", "Duct Tape", 969950014104242),
    846519070449764: ("outerwear_glue", "Epoxy Glue", 170518958309748),
    987128523726831: ("Patch_outerwear", "Sewing Kit", 423981573339098),
}
# Original player events read these item instance variables.
WARMTH_VARIABLE = 8
PROTECTION_VARIABLES = (12, 13, 14, 15)
EQUIPMENT_RANGES = ((133, 136), (250, 277), (300, 307), (350, 368), (400, 415))

def first_value(values, index):
    if index >= len(values) or not isinstance(values[index], list) or not values[index]:
        return None
    return values[index][0]

def is_equipment(item_id):
    return any(low <= item_id <= high for low, high in EQUIPMENT_RANGES)

def find_event(entries, uid):
    for event in entries if isinstance(entries, list) else []:
        if isinstance(event, list) and len(event) > 7 and event[0] == 0:
            if event[4] == uid:
                return event
            found = find_event(event[7], uid)
            if found is not None:
                return found
    return None

def walk_events(event):
    if not isinstance(event, list) or len(event) < 8 or event[0] != 0:
        return
    yield event
    for child in event[7] if isinstance(event[7], list) else []:
        yield from walk_events(child)

def source_outerwear_repairs(project, items_by_name):
    sheets = project.get("project", [None] * 7)[6]
    game = next((sheet for sheet in sheets if isinstance(sheet, list) and
                 sheet and sheet[0] == "Game_events"), None)
    if game is None:
        raise SystemExit("Original Game_events sheet missing for outerwear repairs")
    rows = []
    for uid, (label, item_name, gain_event_uid) in REPAIR_EVENTS.items():
        event = find_event(game[1], uid)
        if event is None:
            raise SystemExit(f"Missing original outerwear repair event {uid}")
        conditions = event[5] if len(event) > 5 and isinstance(event[5], list) else []
        if not any(
            isinstance(condition, list) and len(condition) > 9 and
            condition[0] == 292 and condition[1] == 71 and
            condition[9] == [[10, 0], [8, 0], [7, [2, label]]]
            for condition in conditions
        ):
            raise SystemExit(f"Outerwear repair event {uid} no longer matches {label}")
        item_id = items_by_name.get(item_name)
        if item_id is None:
            raise SystemExit(f"Original item name missing for repair material: {item_name}")
        gain_event = find_event(game[1], gain_event_uid)
        if gain_event is None:
            raise SystemExit(f"Missing original repair amount event {gain_event_uid}")
        gains = []
        for action in gain_event[6] if isinstance(gain_event[6], list) else []:
            if not isinstance(action, list) or len(action) < 6 or action[0:2] != [1030, 88]:
                continue
            params = action[5]
            variable = next((p[1] for p in params if isinstance(p, list) and len(p) > 1 and p[0] == 10), None)
            value = next((p[1] for p in params if isinstance(p, list) and len(p) > 1 and p[0] == 7), None)
            if variable != 6 or not isinstance(value, list) or len(value) != 2 or value[0] != 4:
                continue
            if not isinstance(value[1], list) or len(value[1]) != 2:
                continue
            left, right = value[1]
            if left == [21, 1030, False, None, 6] and isinstance(right, list) and len(right) == 2 and right[0] == 0:
                gains.append(right[1])
        if len(gains) != 1 or not isinstance(gains[0], int) or gains[0] <= 0 or gains[0] > 100:
            raise SystemExit(f"Unexpected original repair gain for {label}: {gains}")
        rows.append((item_id, gains[0], label))
    rows.sort()
    if len({item_id for item_id, _, _ in rows}) != len(rows):
        raise SystemExit("Duplicate item ID in original outerwear repair materials")
    return rows

def main():
    for path in (XML, PROJECT):
        if not path.is_file():
            raise SystemExit(f"Missing original source: {path}")
    xml_items = ET.parse(XML).getroot().findall("item")
    item_ids = {int(item.findtext("id", "-1").strip()) for item in xml_items}
    items_by_name = {item.findtext("name", "").strip(): int(item.findtext("id", "-1").strip()) for item in xml_items}
    project_text = PROJECT.read_text(encoding="utf-8-sig")
    project_text = re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", project_text)
    project = json.loads(project_text)
    properties = {}
    item_conditions = {}
    for layout in project["project"][5]:
        for layer in layout[6]:
            instances = layer[14] if len(layer) > 14 and isinstance(layer[14], list) else []
            for instance in instances:
                if not isinstance(instance, list) or len(instance) < 4:
                    continue
                values = instance[3]
                if not isinstance(values, list):
                    continue
                # Item instances store their original MDZ item ID in variable 16.
                item_id = first_value(values, 16)
                if not isinstance(item_id, int) or item_id <= 0 or item_id not in item_ids:
                    continue

                condition_value = first_value(values, 6)
                if isinstance(condition_value, (int, float)):
                    condition = int(condition_value)
                    if condition < 0 or condition > 100:
                        raise SystemExit(f"Invalid original item condition for {item_id}: {condition}")
                    previous_condition = item_conditions.get(item_id)
                    if previous_condition is not None and previous_condition != condition:
                        raise SystemExit(
                            f"Conflicting original item condition for {item_id}: "
                            f"{previous_condition} vs {condition}"
                        )
                    item_conditions[item_id] = condition

                if not is_equipment(item_id):
                    continue
                warmth_value = first_value(values, WARMTH_VARIABLE)
                warmth = float(warmth_value) if isinstance(warmth_value, (int, float)) else 0.0
                protection = tuple(
                    float(value) if isinstance((value := first_value(values, variable)), (int, float)) else 0.0
                    for variable in PROTECTION_VARIABLES
                )
                if warmth == 0.0 and not any(protection):
                    continue
                values_tuple = (warmth, *protection)
                previous = properties.get(item_id)
                if previous is not None and previous != values_tuple:
                    raise SystemExit(
                        f"Conflicting original equipment values for item {item_id}: "
                        f"{previous} vs {values_tuple}"
                    )
                properties[item_id] = values_tuple

    rows = sorted(properties.items())
    if len(rows) < 30:
        raise SystemExit(f"Unexpectedly low source equipment coverage: {len(rows)}")
    lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::items {",
        "struct SourceItemProtection { std::uint16_t item_id; float warmth, helmet, outerwear, vest, pants; };",
        f"inline constexpr std::size_t kSourceItemProtectionCount = {len(rows)};",
        "inline constexpr SourceItemProtection kSourceItemProtection[] = {",
    ]
    for item_id, values in rows:
        formatted = []
        for value in values:
            text = f"{value:.9g}"
            if "." not in text and "e" not in text:
                text += ".0"
            formatted.append(text + "f")
        lines.append(f"    {{{item_id}, {', '.join(formatted)}}},")
    lines += ["};", "} // namespace minidayz::items", ""]

    condition_rows = sorted(item_conditions.items())
    repair_rows = source_outerwear_repairs(project, items_by_name)
    if len(condition_rows) < 150:
        raise SystemExit(f"Unexpectedly low original item condition coverage: {len(condition_rows)}")
    condition_lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::items {",
        "struct SourceItemCondition { std::uint16_t item_id; std::uint8_t condition; };",
        f"inline constexpr std::size_t kSourceItemConditionCount = {len(condition_rows)};",
        "inline constexpr SourceItemCondition kSourceItemConditions[] = {",
    ]
    for item_id, condition in condition_rows:
        condition_lines.append(f"    {{{item_id}, {condition}}},")
    condition_lines += ["};", "} // namespace minidayz::items", ""]

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    CONDITION_OUTPUT.write_text("\n".join(condition_lines), encoding="utf-8")
    repair_lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::items {",
        "struct SourceOuterwearRepair { std::uint16_t item_id; std::uint8_t condition_gain; };",
        f"inline constexpr std::size_t kSourceOuterwearRepairCount = {len(repair_rows)};",
        "inline constexpr SourceOuterwearRepair kSourceOuterwearRepairs[] = {",
    ]
    for item_id, gain, _label in repair_rows:
        repair_lines.append(f"    {{{item_id}, {gain}}},")
    repair_lines += ["};", "} // namespace minidayz::items", ""]
    REPAIR_OUTPUT.write_text("\n".join(repair_lines), encoding="utf-8")
    print(
        f"Generated {len(rows)} original equipment property rows and "
        f"{len(condition_rows)} item condition defaults and "
        f"{len(repair_rows)} source outerwear repair materials"
    )

if __name__ == "__main__":
    main()
