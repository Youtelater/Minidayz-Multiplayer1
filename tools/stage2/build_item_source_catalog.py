#!/usr/bin/env python3
"""Generate a complete source-backed native item catalog.

Names come from the original English localization XML; object identities come
from MDZItemTable and the original Construct object table. Gameplay effect
fields are deliberately not synthesized here: their authoritative values
must be translated from the original event conditions/actions.
"""
from __future__ import annotations
import json, re, re, xml.etree.ElementTree as ET
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
XML=ROOT/"Minidayz-Multiplayer-main/l_eng_items.xml"
TABLE=ROOT/"mp_itemtable.js"
PROJECT=ROOT/"data/stage1/construct_project_parsed.json"
OUT=ROOT/"include/minidayz/generated/item_source_catalog.hpp"

def main():
    for p in (XML,TABLE,PROJECT):
        if not p.is_file(): raise SystemExit(f"Missing original source: {p}")
    names={}
    for item in ET.parse(XML).getroot().findall("item"):
        i=int(item.findtext("id","-1").strip())
        if i in names: raise SystemExit(f"Duplicate item id {i}")
        names[i]=item.findtext("name","").strip()
    if len(names)!=405 or 0 not in names: raise SystemExit(f"Unexpected item count: {len(names)}")
    src=re.sub(r"\\u([0-9a-fA-F]{4})",lambda m:chr(int(m.group(1),16)),TABLE.read_text(encoding="utf-8"))
    marker=src.find("MDZItemTable"); start=src.find("{",marker); end=src.find("}",start)
    if min(marker,start,end)<0: raise SystemExit("Original MDZItemTable missing")
    object_names={int(m.group(1)):json.loads(m.group(2)) for m in re.finditer(r'(\d+)\s*:\s*("[^"]*")',src[start+1:end])}
    project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", PROJECT.read_text(encoding="utf-8")))
    objects=project["project"][3]
    indices={}
    for n,obj in enumerate(objects):
        if isinstance(obj,list) and obj and isinstance(obj[0],str): indices.setdefault(obj[0],n)
    rows=[]
    for item_id,name in sorted(names.items()):
        object_name=object_names.get(item_id,"")
        object_index=indices.get(object_name,65535)
        rows.append((item_id,name,object_index))
    lines=["#pragma once","#include <cstddef>","#include <cstdint>",
      "namespace minidayz::items {",
      "struct SourceItemDefinition { std::uint16_t id; const char* name; std::uint16_t construct_object; };",
      f"inline constexpr std::size_t kSourceItemDefinitionCount = {len(rows)};",
      "inline constexpr SourceItemDefinition kSourceItemDefinitions[] = {"]
    for i,n,o in rows: lines.append(f"    {{{i}, {json.dumps(n,ensure_ascii=False)}, {o}}},")
    lines += ["};","} // namespace minidayz::items",""]
    OUT.parent.mkdir(parents=True,exist_ok=True); OUT.write_text("\n".join(lines),encoding="utf-8")
    unresolved=sum(o==65535 for _,_,o in rows)
    print(f"Generated {len(rows)} original item definitions; {len(rows)-unresolved} have Construct object mappings; {unresolved} unresolved.")

if __name__=="__main__": main()
