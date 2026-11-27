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
    # Description text IDs are NOT item IDs. For example the original
    # comment "Improvised Bow" describes text ID 243 but item ID 163, and
    # "Civilian Tent" describes text ID 307 but item ID 456. Bind entries
    # only through matching source XML item-name annotations, never by
    # assuming the two unrelated numeric ID namespaces coincide.
    names_by_id = {int(row.findtext("id", "-1").strip()):
                   (row.findtext("name") or "").strip()
                   for row in ET.parse(ITEM_XML).getroot().findall("item")}
    ids_by_name: dict[str, list[int]] = {}
    for iid, label in names_by_id.items():
        if label:
            ids_by_name.setdefault(label.casefold(), []).append(iid)
    parser = ET.XMLParser(target=ET.TreeBuilder(insert_comments=True))
    annotated_xml = ET.parse(DESCRIPTION_XML, parser=parser).getroot()
    annotation_aliases = {
        "feature- equivalent exchange": "equivalent exchange",
        "feature- item carryover": "item carryover",
        "40mm grenade": "40mm grenade shot",
        "vog-25 grenade": "vog-25 grenade shot",
        "m203": "m203 grenade launcher",
        "gp-25": "gp-25 grenade launcher",
        "laser sights": "laser sight",
        "keycard>": "keycard",
        "milkor mgl❓": "milkor mgl",
        "colt m16a2": "m16a2",
        "mini sight": "mini sights",
        "fried egg": "fried eggs",
        "-work gloves": "work gloves",
    }
    annotated: dict[int, list[tuple[int, str]]] = {}
    comment = ""
    for child in annotated_xml:
        if child.tag is ET.Comment:
            comment = (child.text or "").strip()
        elif child.tag == "text":
            text_id = int(child.findtext("id", "-1").strip())
            key=annotation_aliases.get(comment.casefold(),comment.casefold())
            if key in ids_by_name and descriptions.get(text_id, "").strip():
                for iid in ids_by_name[key]:
                    annotated.setdefault(iid, []).append((text_id, descriptions[text_id]))
            if key=="ammo boxes" and text_id==761:
                # A single source annotation introduces this group of
                # identical descriptions for the distinct ammo-box IDs.
                box_ids=ids_by_name["ammo box"]
                assert set(box_ids)==set(range(761,777))
                for iid in box_ids:
                    assert descriptions[iid].strip()==descriptions[text_id].strip()
                    annotated.setdefault(iid, []).append((iid, descriptions[iid]))
            comment = ""
    ambiguous = sorted(iid for iid, rows in annotated.items()
                       if len({text.strip() for _,text in rows}) != 1)
    verified = sorted((iid, rows[0][0], rows[0][1])
                      for iid, rows in annotated.items() if iid not in ambiguous)
    if not verified or ambiguous:
        if ambiguous:
            print(f"Skipped ambiguous original annotation item IDs: {ambiguous}")
        if not verified:
            raise SystemExit("No unambiguous XML comment-to-name mappings found")

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
        lines.append("    {" + str(item_id) + ", " + json.dumps(description, ensure_ascii=False) + "},")
    lines += ["};",
              "struct VerifiedItemDescription { std::uint16_t item_id; std::uint16_t source_text_id; const char* description; };",
              f"inline constexpr std::size_t kVerifiedItemDescriptionCount = {len(verified)};",
              "inline constexpr VerifiedItemDescription kVerifiedItemDescriptions[] = {"]
    for item_id, text_id, description in verified:
        lines.append("    {" + str(item_id) + ", " + str(text_id) + ", " +
                     json.dumps(description, ensure_ascii=False) + "},")
    lines += ["};", f"inline constexpr std::size_t kItemsWithoutSourceDescription = {len(missing)};",
              "} // namespace minidayz::items", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {len(descriptions)} raw description rows, {len(verified)} verified item-name annotation mappings")
    print(f"Item IDs with no original description ({len(missing)}): {missing}")
    print(f"Description IDs not present in item-name catalog ({len(extra)}): {extra}")

if __name__ == "__main__":
    main()
