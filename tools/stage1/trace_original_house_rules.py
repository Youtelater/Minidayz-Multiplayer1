#!/usr/bin/env python3
"""Print root-original House_transparenting event actions/conditions.

Purpose: translate original building roof/interior rules without guessing
distance thresholds, sprites, audio or per-building collision.
"""
from pathlib import Path
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
project=decode((ROOT/"data.deobfuscated.js").read_bytes())
target=5249390718058362
def walk(nodes,sheet,anc=(),path=(),active=False,depth=0):
    for i,e in enumerate(nodes):
        if not isinstance(e,list) or len(e)<8 or e[0]!=0:continue
        here=path+(i,)
        on=active or e[4]==target
        if on:
            print("ROOF_RULE",json.dumps({
                "sheet":sheet,"path":here,"uid":e[4],
                "depth":depth,"parents":anc[-5:],
                "conditions":e[5],"actions":e[6],
                "child_count":len(e[7])
            },ensure_ascii=False,separators=(",",":"))[:10000])
        if on and depth>=4:continue
        walk(e[7],sheet,anc+(e[4],),here,on,depth+1 if on else 0)
for sh in project[6]:
    walk(sh[1],sh[0])
