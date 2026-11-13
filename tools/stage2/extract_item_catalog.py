#!/usr/bin/env python3
"""Generate the Stage 2 item catalog from the original English item XML."""
from __future__ import annotations
import json
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Minidayz-Multiplayer-main/l_eng_items.xml"
OUTPUT = ROOT / "include/minidayz/generated/item_catalog.hpp"

def main() -> None:
    if not SOURCE.is_file():
        raise SystemExit(f"Original item catalog is missing: {SOURCE}")
    root = ET.parse(SOURCE).getroot()
    rows: list[tuple[int, str]] = []
    seen: set[int] = set()
    for item in root.findall("item"):
        raw_id = item.findtext("id")
        name = item.findtext("name")
        if raw_id is None or name is None:
            raise SystemExit("Item entry is missing its id or localized name")
        item_id = int(raw_id.strip())
        if item_id < 0 or item_id > 65535 or item_id in seen:
            raise SystemExit(f"Invalid or duplicate item id: {item_id}")
        seen.add(item_id)
        rows.append((item_id, name.strip()))
    rows.sort()
    if len(rows) != 405 or not rows or rows[0] != (0, ""):
        raise SystemExit(f"Unexpected original item catalog: {len(rows)} entries")
    lines = [
        "#pragma once",
        "#include <cstddef>",
        "#include <cstdint>",
        "namespace minidayz::items {",
        "struct ItemDefinition { std::uint16_t id; const char* name; };",
        f"inline constexpr std::size_t kItemDefinitionCount = {len(rows)};",
        "inline constexpr ItemDefinition kItemDefinitions[] = {",
    ]
    for item_id, name in rows:
        lines.append(f"    {{{item_id}, {json.dumps(name, ensure_ascii=False)}}},")
    lines += ["};", "} // namespace minidayz::items", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {len(rows)} original item definitions from {SOURCE.relative_to(ROOT)}")

if __name__ == "__main__":
    main()
