#!/usr/bin/env python3
"""Locate original Construct ACE events which play the ROMFS Stage 2 sounds."""
import json
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/"data/stage1/construct_project_parsed.json").read_text(encoding="utf-8-sig")
project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",source))
names={"crafting","drop1","drop2","geardrop1","geardrop2","zippero","empty_click"}
def walk(events,parents=()):
    for e in events if isinstance(events,list) else ():
        if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
        yield e,parents
        yield from walk(e[7] if len(e)>7 else [],(*parents,e))
for sheet in project["project"][6]:
    if not isinstance(sheet,list) or len(sheet)<2:continue
    for e,parents in walk(sheet[1]):
        actions=json.dumps(e[6],separators=(",",":")).lower()
        hits={name for name in names if re.search(r'(?i)\"'+re.escape(name)+r'\"',actions)}
        if not hits:continue
        print("SOURCE_AUDIO",json.dumps({
            "names":sorted(hits),"sheet":sheet[0],"event_uid":e[4],
            "parent_uids":[p[4] for p in parents[-5:]],
            "conditions":e[5],"actions":e[6],
        },separators=(",",":"),ensure_ascii=False)[:4500])
