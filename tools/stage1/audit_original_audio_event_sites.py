#!/usr/bin/env python3
"""Index all root-original Construct audio action sites against available WAVs.

This is source/cue packaging evidence, NOT proof the 3DS runtime dispatches an event.
"""
from __future__ import annotations
import argparse,csv,json,re,sys
from collections import Counter
from pathlib import Path
from audit_original_world_exhaustive import ROOT,decode,strings

SIGNATURES={(187,81),(187,398)}
def main():
 p=argparse.ArgumentParser()
 p.add_argument("--out",type=Path,default=ROOT/"audit-output/original-audio")
 opts=p.parse_args();opts.out.mkdir(parents=True,exist_ok=True)
 original=decode((ROOT/"data.deobfuscated.js").read_bytes())
 wav={x.stem.casefold():x.name for x in (ROOT/"romfs/wav").glob("*.wav")}
 native=(ROOT/"source/stage2_audio.cpp").read_text(encoding="utf-8")
 scene=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
 rows=[];group_counts=Counter();unmatched=Counter()
 def scan(entries,sheet,ancestor=(),group_chain=(),path=()):
  for ordinal,e in enumerate(entries):
   if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
   chain=group_chain
   if isinstance(e[1],list) and len(e[1])>1 and isinstance(e[1][1],str):
    chain=group_chain+(e[1][1],)
   ownpath=path+(ordinal,)
   conditions=e[5] if isinstance(e[5],list) else []
   actions=e[6] if isinstance(e[6],list) else []
   for ai,a in enumerate(actions):
    if not isinstance(a,list) or len(a)<6 or tuple(a[:2]) not in SIGNATURES:continue
    stems=list(dict.fromkeys(strings(a[5:])))
    known=[s for s in stems if s.casefold() in wav]
    missing=[s for s in stems if s.casefold() not in wav]
    for m in missing:unmatched[m]+=1
    group_counts[chain[-1] if chain else "<ungrouped>"]+=1
    rows.append({
      "sheet":sheet,"path":sheet+":"+".".join(map(str,ownpath)),
      "uid":e[4],"ancestor_uids":"|".join(map(str,ancestor)),
      "group_chain":" / ".join(chain),"action_index":ai,
      "command":a[1],"sound_literals":"|".join(stems),
      "matching_wavs":"|".join(wav[s.casefold()] for s in known),
      "unmatched_literals":"|".join(missing),
      "requires_dynamic_resolution":str(not stems or bool(missing)).lower(),
      "raw_conditions":json.dumps(conditions,separators=(",",":")),
      "raw_action":json.dumps(a,separators=(",",":"))
    })
   children=e[7] if len(e)>7 and isinstance(e[7],list) else []
   scan(children,sheet,ancestor+(e[4],),chain,ownpath)
 for name,items in original[6]:scan(items,name)
 if len(rows)!=461:
  raise SystemExit(f"Original audio source changed: expected 461 action sites, got {len(rows)}")
 with (opts.out/"original_audio_event_sites.csv").open("w",newline="",encoding="utf-8") as f:
  w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
 summary={
  "original_audio_action_sites":len(rows),
  "audio_actions_with_literal_matching_wav":sum(bool(x["matching_wavs"]) for x in rows),
  "audio_actions_requiring_dynamic_or_nonwav_review":sum(x["requires_dynamic_resolution"]=="true" for x in rows),
  "available_converted_wavs":len(wav),
  "nonwav_literal_candidates":dict(unmatched),
  "top_source_groups":group_counts.most_common(50),
  "native_audio_playback_implemented":("Stage2Audio::play_original_world_sound" in native),
  "live_door_audio_callsite_present":("play_original_world_sound(" in scene),
  "live_tree_hit_audio_verified":False,
  "note":"Source action matching is NOT equivalent to 3DS runtime event coverage. The underlying expression can select a cue dynamically."
 }
 (opts.out/"summary.json").write_text(json.dumps(summary,indent=2)+"\n",encoding="utf-8")
 report=["# Original MiniDAYZ audio event site inventory","",
 f"- Original Construct direct audio action sites: **{len(rows)}**",
 f"- Converted WAV file count: **{len(wav)}**",
 f"- Sites containing at least one literal matching a WAV name: **{summary['audio_actions_with_literal_matching_wav']}**",
 f"- Sites requiring dynamic or non-WAV expression review: **{summary['audio_actions_requiring_dynamic_or_nonwav_review']}**","",
 "Raw full original source conditions, group ancestry, action payloads, chosen-literal candidates and WAV matches are retained in `original_audio_event_sites.csv`.","",
 "**A WAV being found does NOT prove a 3DS game-event handler plays it.** Actual playback, variants, probability, volume, loop, stop and 3DS test evidence must be checked separately.","",
 "## Sound-bearing groups (top 50, overlapping group labels are not expanded)","",
 "| Group | Direct sound action sites |","|---|---:|"]
 report += [f"| {name.replace('|','/')} | {count} |" for name,count in summary["top_source_groups"]]
 (opts.out/"README.md").write_text("\n".join(report)+"\n",encoding="utf-8")
 print(json.dumps(summary,indent=2))
if __name__=="__main__":main()
