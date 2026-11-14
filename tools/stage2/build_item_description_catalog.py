#!/usr/bin/env python3
"""Generate the original item-description catalog from root-level l_eng_new.xml."""
from __future__ import annotations
import json
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DESCRIPTION_XML = ROOT / "l_eng_new.xml"
ITEM_XML = ROOT / "l_eng_items.xml"
OUTPUT = ROOT / "include/minidayz/generated/item_description_catalog.hpp"

def main() -> None:
    for path in (DESCRIPTION_XML, ITEM_XML):
        if not path.is_file():
            raise SystemExit(f"Missing root-level original source: {path.relative_to(ROOT)}")
    descriptions: dict[int, str] = {}
    for row in ET.parse(DESCRIPTION_XML).getroot().findall("text"):
        raw_id, text = row.findtext("id"), row.findtext("name")
        if raw_id is None or text is None:
            raise SystemExit("Original description row is missing id/name")
        item_id = int(raw_id.strip())
        if item_id in descriptions:
            raise SystemExit(f"Duplicate original description id: {item_id}")
        descriptions[item_id] = text
    item_ids = {int(row.findtext("id", "-1").strip())
                for row in ET.parse(ITEM_XML).getroot().findall("item")}
    if len(descriptions) != 528 or len(item_ids) != 405:
        raise SystemExit(f"Unexpected original source counts: {len(descriptions)} descriptions, {len(item_ids)} items")
    missing = sorted(item_ids - descriptions.keys())
    extra = sorted(descriptions.keys() - item_ids)
    lines = [
        "#pragma once", "#include <cstddef>", "#include <cstdint>",
        "namespace minidayz::items {",
        "struct SourceItemDescription { std::uint16_t item_id; const char* description; };",
        f"inline constexpr std::size_t kSourceItemDescriptionCount = {len(descriptions)};",
        "inline constexpr SourceItemDescription kSourceItemDescriptions[] = {",
    ]
    for item_id, description in sorted(descriptions.items()):
        lines.append(f"    {{{item_id}, {json.dumps(description, ensure_ascii=False)}},")
    lines += ["};", f"inline constexpr std::size_t kItemsWithoutSourceDescription = {len(missing)};",
              "} // namespace minidayz::items", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {len(descriptions)} exact original item descriptions")
    print(f"Item IDs with no original description ({len(missing)}): {missing}")
    print(f"Description IDs not present in item-name catalog ({len(extra)}): {extra}")

if __name__ == "__main__":
    main()
