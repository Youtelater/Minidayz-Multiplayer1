#!/usr/bin/env python3
"""Extract armor values from original Construct item instances in the maps."""
from __future__ import annotations
import json
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
XML = ROOT / "Minidayz-Multiplayer-main/l_eng_items.xml"
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
OUTPUT = ROOT / "include/minidayz/generated/item_equipment_catalog.hpp"
# The original player hit event reads these four item instance variables.
WARMTH_VARIABLE = 8
PROTECTION_VARIABLES = (12, 13, 14, 15)
EQUIPMENT_RANGES = ((133, 136), (250, 277), (300, 307), (350, 368), (400, 415))

def first_value(values, index):
    if index >= len(values) or not isinstance(values[index], list) or not values[index]:
        return None
    return values[index][0]

def is_equipment(item_id):
    return any(low <= item_id <= high for low, high in EQUIPMENT_RANGES)

def main():
    for path in (XML, PROJECT):
        if not path.is_file():
            raise SystemExit(f"Missing original source: {path}")
    item_ids = {
        int(item.findtext("id", "-1").strip())
        for item in ET.parse(XML).getroot().findall("item")
    }
    project = json.loads(PROJECT.read_text(encoding="utf-8"))
    properties = {}
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
                if not isinstance(item_id, int) or item_id not in item_ids or not is_equipment(item_id):
                    continue
                warmth_value = first_value(values, WARMTH_VARIABLE)
                warmth = float(warmth_value) if isinstance(warmth_value, (int, float)) else 0.0
                protection = tuple(
                    float(value) if isinstance((value := first_value(values, variable)), (int, float)) else 0.0
                    for variable in PROTECTION_VARIABLES
                )
                if warmth == 0.0 and not any(protection):
                    continue
                previous = properties.get(item_id)
                values_tuple = (warmth, *protection)
                if previous is not None and previous != values_tuple:
                    raise SystemExit(f"Conflicting original equipment values for item {item_id}: {previous} vs {values_tuple}")
                properties[item_id] = values_tuple

    rows = sorted(properties.items())
    if len(rows) < 30:
        raise SystemExit(f"Unexpectedly low source armor coverage: {len(rows)}")
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
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {len(rows)} exact original item armor values from map instances")

if __name__ == "__main__":
    main()
