#!/usr/bin/env python3
"""Fail CI if tent storage identity/permanence is not in the root originals."""
import xml.etree.ElementTree as ET
from pathlib import Path
root = Path(__file__).resolve().parents[2]
items = {int(row.findtext("id")): (row.findtext("name") or "").strip()
         for row in ET.parse(root / "l_eng_items.xml").getroot().findall("item")}
assert items.get(456) == "Civilian Tent", "Original tent item identity changed"
ui = [(row.findtext("name") or "").strip()
      for row in ET.parse(root / "l_eng_ui.xml").getroot().findall("text")]
assert any("Civilian tents can be deployed" in line and
           "store items" in line and "permanently" in line
           for line in ui), "Original root tent storage description changed"
print("Verified original root item 456 and permanent tent storage description")
