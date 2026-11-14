#!/usr/bin/env python3
"""Extract armor values from original Construct item instances in the maps."""
from __future__ import annotations
import json, re, xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
XML = ROOT / "Minidayz-Multiplayer-main/l_eng_items.xml"
TABLE = ROOT / "mp_itemtable.js"
PROJECT = ROOT / "data/stage1/construct_project_parsed.json"
OUTPUT = ROOT / "include/minidayz/generated/item_equipment_catalog.hpp"
# The original player hit event reads these four item instance variables.
PROTECTION_VARIABLES = (12, 13, 14, 15)

def first_value(values, index):
    if index >= len(values) or not isinstance(values[index], list) or not values[index]:
        return None
    return values[index][0]

def main():
    for path in (XML, TABLE, PROJECT):
        if not path.is_file():
            raise SystemExit(f"Missing original source: {path}")
    item_ids = {
        int(item.findtext("id", "-1").strip())
        for item in ET.parse(XML).getroot().findall("item")
    }
    table = re.sub(r"\\u([0-9a-fA-F]{4})",
                   lambda m: chr(int(m.group(1), 16)),
                   TABLE.read_text(encoding="utf-8"))
    marker = table.find("MDZItemTable")
    start = table.find("{", marker)
    end = table.find("}", start)
    if min(marker, start, end) < 0:
        raise SystemExit("Original MDZItemTable missing")
    object_names = {
        int(match.group(1)): json.loads(match.group(2))
        for match in re.finditer(r'(\\d+)\\s*:\\s*("[^"]*")', table[start + 1:end])
    }
    project = json.loads(PROJECT.read_text(encoding="utf-8"))
    objects = project["project"][3]
    object_indices = {}
    for index, obj in enumerate(objects):
        if isinstance(obj, list) and obj and isinstance(obj[0], str):
            object_indices.setdefault(obj[0], index)
    object_to_item = {}
    for item_id, object_name in object_names.items():
        if item_id in item_ids:
            object_index = object_indices.get(object_name)
            if object_index is not None:
                object_to_item[object_index] = item_id

    properties = {}
    for layout in project["project"][5]:
        for layer in layout[6]:
            instances = layer[14] if len(layer) > 14 and isinstance(layer[14], list) else []
            for instance in instances:
                if not isinstance(instance, list) or len(instance) < 4:
                    continue
                object_index = instance[1]
                item_id = object_to_item.get(object_index)
                values = instance[3]
                if item_id is None or not isinstance(values, list):
                    continue
                instance_item_id = first_value(values, 16)
                if instance_item_id != item_id:
                    continue
                protection = tuple(
                    float(value) if isinstance((value := first_value(values, variable)), (int, float)) else 0.0
                    for variable in PROTECTION_VARIABLES
                )
                if not any(protection):
                    continue
                previous = properties.get(item_id)
                if previous is not None and previous != protection:
                    raise SystemExit(f"Conflicting original armor values for item {item_id}: {previous} vs {protection}")
                properties[item_id] = protection

    rows = sorted(properties.items())
    if len(rows) < 30:
        raise SystemExit(f"Unexpectedly low source armor coverage: {len(rows)}")
    lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::items {",
        "struct SourceItemProtection { std::uint16_t item_id; float helmet, outerwear, vest, pants; };",
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
