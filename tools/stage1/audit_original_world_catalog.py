#!/usr/bin/env python3
"""Root MiniDAYZ world inventory, with exact IDs and NO inferred live house count."""
import argparse,csv,json,re
from pathlib import Path
from collections import Counter,defaultdict
from audit_original_world_exhaustive import ROOT,decode

PATTERNS={
 "houses_buildings":r"house|building|roof|apartment|shed|garage|cabin|hut|shop|store|hospital|firestation|police|warehouse|barrack|tower|church|military",
 "interiors_furniture":r"_int|interior|furniture|table|chair|bed|sofa|shelf|cabinet|locker|fridge|stove|desk|drawer|container|wardrobe",
 "doors_walls_fences":r"door|gate|wall|fence|barri|wire|obst|window|collision",
 "terrain_roads_water":r"terrain|ground|water|river|lake|sand|snow|swamp|road|asphalt|shore|bridge|tile",
 "trees_plants_bushes":r"tree|forest|bush|berry|plant|flower|stump|wood|log|spruce|pine|oak|leaf",
 "bunkers":r"bunker|underground|basement",
 "vehicles_wrecks":r"car|vehicle|truck|bus|van|uaz|volga|helic|wreck|jeep",
 "loot_items_storage":r"loot|item|drop|ammo|weapon|crate|stash|bag|tent|barrel",
 "spawns_entities":r"spawn|resp|npc|zed|zombie|wolf|deer|rabbit|crow|chicken|dog|bot",
 "weather_world_effects":r"rain|snow|light|night|shadow|fog|fire|smoke|weather",
}
def write_csv(path,rows):
 if not rows:return
 with path.open("w",encoding="utf-8",newline="") as f:
  w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
def events(root):
 stack=[root]
 while stack:
  e=stack.pop()
  if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
  yield e
  stack.extend(reversed(e[7] if len(e)>7 and isinstance(e[7],list) else []))
def main():
 ap=argparse.ArgumentParser()
 ap.add_argument("--out",type=Path,default=ROOT/"audit-output/world/detailed-catalog")
 args=ap.parse_args();out=args.out;out.mkdir(parents=True,exist_ok=True)
 p=decode((ROOT/"data.deobfuscated.js").read_bytes());objs=p[3]
 names={i:o[0] for i,o in enumerate(objs)}
 matches=defaultdict(list);objects=[]
 for i,o in enumerate(objs):
  tags=[k for k,pattern in PATTERNS.items() if re.search(pattern,o[0],re.I)]
  for tag in tags:matches[tag].append(i)
  anim=o[7] if len(o)>7 and isinstance(o[7],list) else []
  frames=sum(len(a[7]) for a in anim if isinstance(a,list) and len(a)>7 and isinstance(a[7],list))
  objects.append(dict(id=i,name=o[0],type=o[1],animations=len(anim),frames=frames,name_matching_candidates="|".join(tags) or "unclassified"))
 write_csv(out/"all_original_objects.csv",objects)
 layers=[];placed=[]
 for li,L in enumerate(p[5]):
  for yi,layer in enumerate(L[6]):
   inst=layer[14] if len(layer)>14 and isinstance(layer[14],list) else []
   layers.append(dict(layout_id=li,layout=L[0],layer_id=yi,layer=layer[0],authoring_instances=len(inst)))
   for x in inst:
    if not isinstance(x,list) or len(x)<3:continue
    t=x[0]
    placed.append(dict(layout=L[0],layer=layer[0],object_id=x[1],object_name=names.get(x[1],"?"),uid=x[2],x=t[0] if len(t)>0 else None,y=t[1] if len(t)>1 else None))
 write_csv(out/"all_layout_layers.csv",layers)
 write_csv(out/"all_editor_placements.csv",placed)
 sheets=dict(p[6]);functions=[e for x in sheets["Game_events"] for e in events(x) if e[4]==5017992314331459]
 if len(functions)!=1:raise RuntimeError("generate_location source function not unique")
 roots=[e for e in functions[0][7] if isinstance(e,list) and e and e[0]==0]
 if len(roots)!=1:raise RuntimeError("generate_location root not unique")
 branch_events=[e for e in roots[0][7] if isinstance(e,list) and e and e[0]==0]
 if len(branch_events)!=62:raise RuntimeError(f"unexpected location branch count: {len(branch_events)}")
 branches=[]
 for idx,e in enumerate(branch_events):
  codes=[]
  for c in e[5] if isinstance(e[5],list) else []:
   params=c[9] if len(c)>9 and isinstance(c[9],list) else []
   last=params[-1] if params else None
   if c[:2]==[514,71] and isinstance(last,list) and len(last)>1 and last[0]==7 and isinstance(last[1],list) and last[1][0]==0:codes.append(last[1][1])
  actions=[a for ev in events(e) for a in ev[6] if isinstance(a,list) and len(a)>1]
  writes=[a for a in actions if a[:2]==[525,199]]
  creates=[a for a in actions if a[:2]==[-1,94]]
  branches.append(dict(index=idx,event_uid=e[4],tile_codes="|".join(map(str,codes)),event_nodes=sum(1 for _ in events(e)),array_writes=len(writes),direct_creates=len(creates),direct_create_parameters=json.dumps([a[5] for a in creates],separators=(",",":"))))
 write_csv(out/"all_62_location_templates.csv",branches)
 # Build a concrete world-element creation matrix from the authoritative
 # create_map_element event function. Do not call all these objects houses:
 # buildings can be made from multiple sprites, families and shadows.
 source_events=[e for root in sheets["Game_events"] for e in events(root)]
 event_uids={e[4] for e in source_events}
 element_rules=[]
 for e in source_events:
  # This function dispatch is distinct from editor-placed palette objects.
  if e[4]!=7736925111997576:continue
  def visit_element(node,ancestor=()):
   for ch in node[7] if len(node)>7 and isinstance(node[7],list) else []:
    if not isinstance(ch,list) or len(ch)<7 or ch[0]!=0:continue
    active=ancestor+(node[4],)
    for a in ch[6] if isinstance(ch[6],list) else []:
     if not isinstance(a,list) or len(a)<6 or a[:2]!=[-1,94]:continue
     # System Create: parameter[0] is the original object expression.
     params=a[5]
     object_ids=[]
     for par in params[:1]:
      if isinstance(par,list) and len(par)>1 and isinstance(par[1],list):
       expr=par[1]
       if len(expr)>1 and expr[0]==0 and isinstance(expr[1],int):
        object_ids.append(expr[1])
     for oid in object_ids:
      if not 0<=oid<len(objs):continue
      element_rules.append(dict(
       event_uid=ch[4],ancestor_uids="|".join(map(str,active)),
       source_object_id=oid,object_name=names[oid],
       source_create_action=json.dumps(a,separators=(",",":")),
       source_conditions=json.dumps(ch[5],separators=(",",":"))))
    visit_element(ch,active)
  visit_element(e)
 if element_rules:
  write_csv(out/"original_world_create_actions.csv",element_rules)
 else:
  # A valid zero-result source trace must produce a header-only CSV.
  # The original world helper may use indirect family creation; never
  # manufacture a direct Create action merely to satisfy the auditor.
  with (out/"original_world_create_actions.csv").open("w",newline="",encoding="utf-8") as file:
   csv.writer(file).writerow(("event_uid","ancestor_uids","source_object_id",
                              "object_name","source_create_action",
                              "source_conditions"))
 # Original Construct families are not sprite objects. Preserve actual
 # membership so roof/interior family IDs are never mistaken for art IDs.
 families=[]
 for family in p[4]:
  if not isinstance(family,list) or len(family)<2:continue
  fid=family[0]
  for oid in family[1:]:
   if isinstance(oid,int) and 0<=oid<len(objs):
    families.append(dict(family_id=fid,object_id=oid,object_name=names[oid],
                        relevant_building_family=fid in (1041,1048,1049)))
 write_csv(out/"original_family_membership.csv",families)
 building_ids=set()
 for item in objects:
  if any(tag in item["name_matching_candidates"] for tag in
         ("houses_buildings","interiors_furniture","doors_walls_fences")):
   building_ids.add(item["id"])
 for item in families:
  if item["family_id"] in (1041,1048,1049):
   building_ids.add(item["object_id"])
 for item in element_rules:
  oid=item["source_object_id"]
  if any(term in item["object_name"].lower() for term in
         ("house","roof","building","door","wall","shelf","table","chair","bed")):
   building_ids.add(oid)
 # Source evidence identifies these object IDs regardless of whether
 # their original display names contain an English building keyword.
 # An original Construct name is NOT a guaranteed semantic category.
 building_ids.update((302,306,311,312,313,321,322,323,324,333,348,349,787))
 building_rows=[]
 for oid in sorted(building_ids):
  o=objs[oid]
  matching_layers=sorted({entry["layer"] for entry in placed if entry["object_id"]==oid})
  members=sorted({entry["family_id"] for entry in families if entry["object_id"]==oid})
  inst_count=sum(entry["object_id"]==oid for entry in placed)
  actions=[entry for entry in element_rules if entry["source_object_id"]==oid]
  frame_refs=sum(len(a[7]) for a in (o[7] if len(o)>7 and isinstance(o[7],list) else [])
                 if isinstance(a,list) and len(a)>7 and isinstance(a[7],list))
  building_rows.append(dict(object_id=oid,name=names[oid],
   frame_references=frame_refs,original_families="|".join(map(str,members)),
   editor_instance_count=inst_count,editor_layers="|".join(matching_layers),
   direct_world_create_action_count=len(actions),
   direct_world_event_uids="|".join(map(str,sorted({x["event_uid"] for x in actions}))),
   caution="candidate only; not automatically a unique building"))
 write_csv(out/"building_object_candidates.csv",building_rows)
 summary=dict(source="root/data.deobfuscated.js",original_object_definitions=len(objs),layouts=len(p[5]),editor_placements=len(placed),location_templates=len(branches),location_array_writes=sum(x["array_writes"] for x in branches),direct_create_actions=sum(x["direct_creates"] for x in branches),name_match_candidates={k:len(v) for k,v in matches.items()},building_object_candidates=len(building_rows),original_world_create_actions=len(element_rules),family_member_pairs=len(families))
 (out/"summary.json").write_text(json.dumps(summary,indent=2)+"\n")
 md=["# MiniDAYZ original-world detailed source catalog","",
 "**Source:** root `data.deobfuscated.js`, not an older cached snapshot or multiplayer override.","",
 "**Counting rule:** A location template is not a house; an exterior sprite and its interior can depict the same building. Editor placements are not runtime procedural spawns. Name-matching is exploratory, never a certified house total.","",
 "## Source totals","",
 f"- Object definitions: **{len(objs):,}**",
 f"- Layouts: **{len(p[5])}**",
 f"- Editor-placed instances: **{len(placed):,}**",
 f"- Location-template branches: **{len(branches)}**",
 f"- Location template Array writes: **{summary['location_array_writes']:,}**",
 f"- Direct Create action sites: **{summary['direct_create_actions']}**","",
 "## Object name candidate groups (overlapping, NOT unique houses)","",
 "| Group | Object definitions with matching names |","|---|---:|"]
 md += [f"| {k} | {len(v)} |" for k,v in matches.items()]
 md += ["","## Every original location template","",
 "| Index | Source event UID | Template codes | Array writes | Direct creates |",
 "|---:|---:|---|---:|---:|"]
 md += [f"| {b['index']} | `{b['event_uid']}` | `{b['tile_codes']}` | {b['array_writes']} | {b['direct_creates']} |" for b in branches]
 md += ["","## Authoring instances by populated layer","",
 "| Layout | Layer | Instances |","|---|---|---:|"]
 md += [f"| {r['layout']} | {r['layer']} | {r['authoring_instances']} |" for r in layers if r["authoring_instances"]]
 md += ["","## Source-specific known object relationships","",
 "- Source-referenced exterior/interior pairs: `321→302`, `322→311`, `323→312`, `324→313`, `333→306`. This is a known subset, NOT all houses.",
 "- Construct IDs `1041`, `1048`, `1049` are families; do not count them as standalone sprite designs.",
 "- Tree object IDs: `184`, `207`, `209`, `576`, `577`, `581`. Berry bush `296`. Automatic door `349`; companion `348`; bunker door `787`.","",
 "## Machine-readable full lists","",
 "- `all_original_objects.csv`: every original object name/ID and animation/frame counts.",
 "- `building_object_candidates.csv`: source building/interior/door candidates, family associations, actual authored layers, Create event references and frame counts; **not a unique house total**.",
 "- `original_family_membership.csv`: every source Construct family and member Sprite, including 1041/1048/1049.",
 "- `original_world_create_actions.csv`: literal-object System Create action sites under original create_map_element with original condition and action arrays.",
 "- `all_62_location_templates.csv`: all tile-code branches and exact Array/Create action counts and direct Create parameters.",
 "- `all_layout_layers.csv`: every authoring layer and placement count.",
 "- `all_editor_placements.csv`: every editor instance UID, object name/ID and X/Y.",
 "- `summary.json`: stable machine-readable totals.","",
 "## Exact house count: outstanding semantic proof","",
 "Resolve the original Array write tuples and `create_map_location`/`create_map_element` interpreter into placed structures; deduplicate exterior/interior and roof objects representing the same house. Then evaluate original source location RNG and quotas. An exact live per-world house count cannot be inferred from the 62 branches alone.",""]
 (out/"README.md").write_text("\n".join(md),encoding="utf-8")
 print(json.dumps(summary,indent=2))
if __name__=="__main__":main()
