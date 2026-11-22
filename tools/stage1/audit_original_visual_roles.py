#!/usr/bin/env python3
"""Source-first visual-role inventory; images existing != images always drawn.

Read the ORIGINAL Construct project and canonical assets/gfx. Distinguish
Map placements, event-created graphics and helpers without inventing texture
visibility from a filename. Keep the raw event conditions as audit evidence.
"""
from pathlib import Path
from collections import Counter,defaultdict
import json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
p=decode((ROOT/"data.deobfuscated.js").read_bytes())
objects,layouts,sheets=p[3],p[5],p[6]
target_names={
 "build_ghost-sheet0.png","build_ghost_obstacle-sheet0.png",
 "big_obstacle_base-sheet0.png","obst_base-sheet0.png",
 "collision_checker_general-sheet0.png","player_collision_base-sheet0.png",
 "player_base-sheet0.png","polygon_target_base-sheet0.png",
 "mapgen-sheet0.png","map_element-sheet0.png",
 "ach_city_counter-sheet0.png","tree_leaves-sheet0.png",
 "tree_block-sheet0.png","tilemap_bunker_collision.png",
 "gui_slot_hider_bp-sheet0.png","noice_debug-sheet0.png"
}
by_png=defaultdict(list)
for oid,obj in enumerate(objects):
 if not isinstance(obj,list) or len(obj)<8 or not isinstance(obj[7],list):continue
 for ai,anim in enumerate(obj[7]):
  if not isinstance(anim,list) or len(anim)<8 or not isinstance(anim[7],list):continue
  for fi,fr in enumerate(anim[7]):
   if not isinstance(fr,list) or len(fr)<6 or not isinstance(fr[0],str):continue
   name=Path(fr[0].replace(chr(92),"/")).name.lower()
   by_png[name].append((oid,ai,fi,str(anim[0]),fr[:11]))
layouts_by_obj=defaultdict(list)
for layout in layouts:
 if not isinstance(layout,list) or len(layout)<7:continue
 for li,layer in enumerate(layout[6]):
  if not isinstance(layer,list) or len(layer)<15:continue
  inst=layer[14] if isinstance(layer[14],list) else []
  for entry in inst:
   if not isinstance(entry,list) or len(entry)<3 or not isinstance(entry[1],int):continue
   record=entry[0]
   layouts_by_obj[entry[1]].append({"layout":layout[0],"layer":li,
      "layer_name":layer[0],"visible":layer[3] if len(layer)>3 else None,
      "uid":entry[2],"transform":record[:9] if isinstance(record,list) else record})
event_refs=defaultdict(list)
def walk(events,sheet,groups=()):
 for e in events if isinstance(events,list) else ():
  if not isinstance(e,list) or len(e)<8 or e[0]!=0:continue
  group=e[1][1] if isinstance(e[1],list) and len(e[1])>1 and isinstance(e[1][1],str) else None
  gn=groups+((group,) if group else ())
  for kind,entries in (("condition",e[5]),("action",e[6])):
   for item in entries if isinstance(entries,list) else ():
    if isinstance(item,list) and len(item)>1 and isinstance(item[0],int) and item[0]>=0:
     event_refs[item[0]].append({"kind":kind,"sheet":sheet,
      "event_uid":e[4],"cmd":item[1],"group":gn[-4:],
      "conditions":e[5],"actions":e[6]})
  walk(e[7],sheet,gn)
for sheet,events in sheets:walk(events,sheet)
summary=Counter()
result={}
for png,frames in by_png.items():
 ids={f[0] for f in frames}
 direct_map=[e for oid in ids for e in layouts_by_obj.get(oid,[])]
 role=("map_instance_present" if any(i["layout"]=="Map" and i["visible"] for i in direct_map)
       else "non_map_or_hidden_layout")
 if any(i["layout"]=="Map" and not i["visible"] for i in direct_map):
  role+=";hidden_map_layer"
 summary[role]+=1
 result[png]={"role":role,"source_frames":len(frames),
   "objects":sorted(ids),
   "map_instances":len([i for i in direct_map if i["layout"]=="Map"]),
   "event_references":sum(len(event_refs[oid]) for oid in ids)}
out=ROOT/"audit-output/graphics/visual_role_inventory.json"
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps({"summary":summary,"images":result},indent=2)+"\n",encoding="utf-8")
print("ORIGINAL_SPRITE_ROLE_SUMMARY",json.dumps({"source_pngs":len(result),"roles":summary},sort_keys=True))
for name in sorted(target_names):
 ids=sorted({r[0] for r in by_png[name]})
 if not ids:
  print("ROLE_TARGET",json.dumps({"png":name,"objects":[],"note":"not a Construct Sprite animation; could be a Tilemap asset"}));continue
 hits=[e for oid in ids for e in event_refs[oid]]
 print("ROLE_TARGET",json.dumps({"png":name,"objects":ids,
   "object_names":[objects[oid][0] for oid in ids],
   "frames":len(by_png[name]),"map_instances":[i for oid in ids for i in layouts_by_obj.get(oid,[]) if i["layout"]=="Map"][:12],
   "events":len(hits),"event_groups":Counter(str(x["group"]) for x in hits).most_common(8)},
   ensure_ascii=False,separators=(",",":"))[:11000])
 if name.startswith(("build_ghost","obst_base","big_obstacle","collision_checker","player_collision","ach_city_counter")):
  for h in hits[:10]:
   print("ROLE_EVENT",name,json.dumps(h,ensure_ascii=False,separators=(",",":"))[:3500])
print("VISUAL_ROLE_INVENTORY",out)

print("SOURCE_TEMPLATE_OPACITY_AUDIT")
for oid in (181,193,207,210,218,302,306,311,312,313,321,322,323,324,333,345,346,347,348,349,404,508,514,571,573,857,931,984,1323,1345,1406):
 entries=[(i["layout"],i["layer_name"],i["visible"],i["transform"][:7]) for i in layouts_by_obj.get(oid,[])]
 print("TEMPLATE_ROLE",json.dumps({"object":oid,"name":objects[oid][0],
   "templates":entries[:10],"total":len(entries)},separators=(",",":")))

for oid in (181,193,218,345,346,347,348,349,404,508,514,571,573,857,984):
 frames=[(n,ai,fi) for n,refs in by_png.items() for ref_oid,ai,fi,_,_ in refs if ref_oid==oid]
 refs=event_refs[oid]
 acts=Counter((str(e["group"]),str(e["cmd"])) for e in refs if e["kind"]=="action")
 print("SOURCE_VISUAL_OBJECT",json.dumps({"object":oid,
     "png_frames":frames[:32],"frame_count":len(frames),
     "actions":acts.most_common(15)},separators=(",",":")))
