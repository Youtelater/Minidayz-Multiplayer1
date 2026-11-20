#!/usr/bin/env python3
"""Audit EVERY Construct event from root original; do not implement gameplay."""
import argparse,csv,hashlib,json,re
from collections import Counter,defaultdict
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
def decode(raw):
    raw=re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",raw.decode("utf-8-sig"))
    project=json.loads(raw)["project"]
    if len(project)<8 or not isinstance(project[6],list):raise ValueError("Invalid Construct source")
    return project
def strings(o):
    hits=[]
    def walk(x):
        if isinstance(x,list):
            if len(x)==2 and x[0]==2 and isinstance(x[1],str): hits.append(x[1])
            for y in x:walk(y)
    walk(o)
    return hits
def group(e):
    x=e[1]
    return x[1] if isinstance(x,list) and len(x)>1 and isinstance(x[1],str) else ""
def csvout(path,header,rows):
    with path.open("w",encoding="utf-8",newline="") as f:
        w=csv.writer(f);w.writerow(header);w.writerows(rows)
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--source",type=Path,default=ROOT/"data.deobfuscated.js")
    ap.add_argument("--out",type=Path,default=ROOT/"audit-output/world")
    args=ap.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    raw=args.source.read_bytes();p=decode(raw)
    snap=b"".join(f.read_bytes() for f in sorted((ROOT/"data/stage1/source_snapshot").glob("part_*.jsonpart")))
    provenance={"source_bytes":len(raw),"source_sha256":hashlib.sha256(raw).hexdigest(),
                "snapshot_bytes":len(snap),"snapshot_sha256":hashlib.sha256(snap).hexdigest()}
    try:provenance["same_project_as_cached_snapshot"]= bool(snap) and p==decode(snap)
    except (ValueError,KeyError,UnicodeError) as e:provenance["snapshot_error"]=str(e)
    stats=Counter();by_sheet=Counter();groups=[];calls=[];sounds=[];visual=[];ids=Counter()
    defs=defaultdict(list);call_sites=defaultdict(list)
    out=args.out
    with (out/"all_original_event_traces.jsonl").open("w",encoding="utf-8") as f:
        def visit(events,sheet,anc,paths,indices):
            for ordinal,e in enumerate(events):
                if not isinstance(e,list) or not e:continue
                if e[0]!=0:stats["non_event_entries"]+=1;continue
                uid=e[4];ids[uid]+=1
                gp=group(e);group_path=paths+([gp] if gp else [])
                cond=e[5] if isinstance(e[5],list) else []
                acts=e[6] if isinstance(e[6],list) else []
                children=e[7] if len(e)>7 and isinstance(e[7],list) else []
                stats["events"]+=1;stats["conditions"]+=len(cond);stats["actions"]+=len(acts);by_sheet[sheet]+=1
                if gp:groups.append([sheet,gp,uid,"|".join(map(str,anc)),"|".join(group_path),len(children)])
                row={"sheet":sheet,"uid":uid,"source_event_path":indices+[ordinal],
                     "parent_uids":anc,"groups":group_path,
                     "flags":e[1:4],"conditions":cond,"actions":acts,
                     "children":[x[4] for x in children if isinstance(x,list) and x and x[0]==0]}
                f.write(json.dumps(row,ensure_ascii=False,separators=(",",":"))+"\n")
                for c in cond:
                    if isinstance(c,list) and len(c)>9 and c[0]==189 and c[1]==40:
                        for name in strings(c[9:]):defs[name].append(uid)
                for a in acts:
                    if not isinstance(a,list) or len(a)<2:continue
                    name_literals=strings(a[5:]);js=json.dumps(a,ensure_ascii=False,separators=(",",":"))
                    if a[0]==189 and a[1]==69:
                        name=name_literals[0] if name_literals else "<dynamic>"
                        calls.append([sheet,uid,"|".join(group_path),name,js]);call_sites[name].append(uid)
                    if a[0]==187 and a[1] in (81,398):
                        sounds.append([sheet,uid,"|".join(group_path),a[1],"|".join(name_literals),js])
                    if a[1] in (72,132,137,163) and a[0]!=-1:
                        visual.append([sheet,uid,"|".join(group_path),a[0],a[1],js])
                visit(children,sheet,anc+[uid],group_path,indices+[ordinal])
        for sheet in p[6]:visit(sheet[1],sheet[0],[],[],[])
    csvout(out/"group_declarations.csv",["sheet","name","uid","ancestor_uids","path","direct_children"],groups)
    csvout(out/"function_calls.csv",["sheet","event_uid","group_path","name_literal","full_action_json"],calls)
    csvout(out/"function_resolution.csv",["name_literal","definition_uids","call_uids","matched_definition"],
           [[n,"|".join(map(str,defs.get(n,[]))),"|".join(map(str,call_sites[n])),bool(defs.get(n))] for n in sorted(call_sites)])
    csvout(out/"audio_actions.csv",["sheet","event_uid","group_path","command_id","sound_literals","full_action_json"],sounds)
    csvout(out/"visual_state_actions.csv",["sheet","event_uid","group_path","object_id","command_id","full_action_json"],visual)
    # Original asset and world layout references, NEVER inferred from generated romfs.
    frame_rows=[]
    for object_id,obj in enumerate(p[3]):
        animations=obj[7] if len(obj)>7 and isinstance(obj[7],list) else []
        for animation_id,animation in enumerate(animations):
            frames=animation[7] if len(animation)>7 and isinstance(animation[7],list) else []
            for frame_id,frame in enumerate(frames):
                if isinstance(frame,list) and frame and isinstance(frame[0],str):
                    frame_rows.append([object_id,obj[0],animation_id,animation[0],frame_id,
                        frame[0],frame[1] if len(frame)>1 else "",
                        frame[2] if len(frame)>2 else "",frame[3] if len(frame)>3 else "",
                        frame[4] if len(frame)>4 else "",frame[5] if len(frame)>5 else "",
                        frame[7] if len(frame)>7 else "",frame[8] if len(frame)>8 else "",
                        json.dumps(frame[10] if len(frame)>10 else None,separators=(",",":"))])
    csvout(out/"original_sprite_frames.csv",
        ["object_id","object_name","animation_id","animation_name","frame_id",
         "source_path","sheet_id","x","y","width","height","origin_x","origin_y","source_collision_json"],
        frame_rows)
    placement_rows=[]
    for layout_id,layout in enumerate(p[5]):
        if len(layout)<=6 or not isinstance(layout[6],list):continue
        for layer_id,layer in enumerate(layout[6]):
            instances=layer[14] if len(layer)>14 and isinstance(layer[14],list) else []
            for instance in instances:
                if not isinstance(instance,list) or len(instance)<3 or not isinstance(instance[0],list):continue
                t=instance[0]
                placement_rows.append([layout_id,layout[0],layer_id,layer[0],instance[1],
                    instance[2],*(t[i] if i<len(t) else "" for i in range(11))])
    csvout(out/"original_layout_instances.csv",
        ["layout_id","layout_name","layer_id","layer_name","object_id","instance_uid",
         "x","y","z","width","height","angle","opacity","scale_x","origin_x","origin_y","remaining_transform"],placement_rows)
    sprite_files={str(pth.relative_to(ROOT)).replace("\\","/"):pth for pth in
                  (ROOT/"assets/gfx").rglob("*.png")}
    by_basename=defaultdict(list)
    for path in sprite_files:by_basename[Path(path).name.lower()].append(path)
    refs=defaultdict(list);unmatched=[]
    for row in frame_rows:
        original=row[5].replace("\\","/")
        candidates=[original,"assets/gfx/"+original,"assets/gfx/"+Path(original).name]
        exact=next((c for c in candidates if c in sprite_files),None)
        if not exact:
            opts=by_basename.get(Path(original).name.lower(),[])
            if len(opts)==1:exact=opts[0]
        if exact:refs[exact].append(row[:5])
        else:unmatched.append([row[0],row[2],row[4],original])
    csvout(out/"original_png_accounting.csv",
        ["png_path","original_frame_reference_count","original_frame_refs_json"],
        [[path,len(refs.get(path,[])),json.dumps(refs.get(path,[]),separators=(",",":"))]
         for path in sorted(sprite_files)])
    csvout(out/"unmatched_original_frame_paths.csv",
        ["object_id","animation_id","frame_id","source_path"],unmatched)
    summary={"source":"data.deobfuscated.js","provenance":provenance,
             "object_types":len(p[3]),"layouts":len(p[5]),"sheets":len(p[6]),
             "sprite_frame_refs":len(frame_rows),"original_layout_instances":len(placement_rows),
             "png_files":len(sprite_files),"pngs_referenced_by_frames":len(refs),
             "unmatched_frame_paths":len(unmatched),
             "events":stats["events"],"conditions":stats["conditions"],"actions":stats["actions"],
             "event_uids_with_duplicates":[k for k,v in ids.items() if v>1][:100],
             "duplicate_uid_count":sum(1 for v in ids.values() if v>1),
             "sheet_events":dict(by_sheet),"group_declarations":len(groups),
             "function_calls":len(calls),"audio_actions":len(sounds),"visual_state_actions":len(visual),
             "function_names_called":len(call_sites),
             "unmatched_static_function_names":sorted(k for k in call_sites if k not in defs)}
    (out/"summary.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False)+"\n")
    md=["# Exhaustive original source event audit","",
        "Authority: root data.deobfuscated.js; source hash: "+provenance["source_sha256"],"",
        "Snapshot is semantically the same source: "+str(provenance.get("same_project_as_cached_snapshot","unknown"))+".",
        "If false, earlier indexes compiled from the cache MUST NOT be used as original-source evidence.","",
        "Events: "+str(stats["events"])+", conditions: "+str(stats["conditions"])+", actions: "+str(stats["actions"])+".",
        "Exact condition/action arrays for each event and ancestor UIDs are stored in all_original_event_traces.jsonl.",
        "See the group, function, sound, and frame/opacity/layer CSVs for traceable source references.",
        "Source graphic rectangles, hotspots and original collision data: original_sprite_frames.csv.",
        "Source layout placement/transform: original_layout_instances.csv.",
        "Every PNG accounted for: original_png_accounting.csv and unmatched_original_frame_paths.csv.","",
        "Extraction is exhaustive at the event syntax level. Numerical action meaning, plugin runtime semantics,",
        "random order, source-asset reachability, and parity with running 3DS are NOT automatically proven.",
        "Do not infer a sound or item reward merely because the asset or call name exists.","",
        "## Event sheets","",
        "| Sheet | Nodes |","|---|---:|"]
    md += ["| "+k+" | "+str(v)+" |" for k,v in by_sheet.items()]
    (out/"README.md").write_text("\n".join(md)+"\n")
    print(json.dumps(summary,indent=2,ensure_ascii=False))
if __name__=="__main__":main()
