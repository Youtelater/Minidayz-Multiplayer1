#!/usr/bin/env python3
"""Inspect root original Construct bed/sleep source, without guessed rules."""
from pathlib import Path
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools"/"stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
TERMS=("SleepApproved","GotoSleep","sleep","Sleep","bed","Bed")
def walk(nodes,parents=()):
 for e in nodes:
  if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
  yield e,parents
  if len(e)>7 and isinstance(e[7],list):
   yield from walk(e[7],parents+(e[4],))
def find_terms(node):
 s=json.dumps(node,ensure_ascii=False,separators=(",",":"))
 return [term for term in TERMS if ('"'+term+'"') in s]
for sheet,events in p[6]:
 for e,parents in walk(events):
  cond=e[5] if isinstance(e[5],list) else []
  act=e[6] if isinstance(e[6],list) else []
  matches=find_terms(cond+act)
  direct_bed=any(isinstance(c,list) and c and c[0] in (951,442) for c in cond+act)
  if not matches and not direct_bed:continue
  print("SOURCE_SLEEP_EVENT",json.dumps({"sheet":sheet,"uid":e[4],
   "ancestors":parents[-5:],"matches":matches,"bed_or_pump":direct_bed,
   "conditions":cond,"actions":act},ensure_ascii=False,separators=(",",":"))[:6000])
