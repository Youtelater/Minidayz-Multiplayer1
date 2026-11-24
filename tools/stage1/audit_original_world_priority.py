#!/usr/bin/env python3
"""Extract root-original evidence for critical world interactions and item IDs."""
from pathlib import Path
import csv,json,re,sys,xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[2]
SRC=ROOT/"data.deobfuscated.js"
OUT=ROOT/"audit-output/world"
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode,strings

WORLD_UIDS={
    8754965976446553:"tree_207_hit",
    2738244684146745:"tree_207_fall",
    5386206472407495:"tree_209_hit",
    7838407830495979:"tree_209_fall",
    6061166023614368:"door_open",
    243854400993751:"door_close",
    5249390718058362:"house_transparency_group",
    5688415260272274:"automatic_door_group",
    5418835019314848:"Spawn_drop",
    8627807469654645:"create_map_location",
    7736925111997576:"create_map_element",
    6835885414231505:"map_generation_group",
    3772264929481614:"loot_respawn_group",
    7952760861768009:"Trigger_spawn",
}
def main():
    p=decode(SRC.read_bytes())
    item_names={}
    for item in ET.parse(ROOT/"l_eng_items.xml").getroot().findall("item"):
        ident=item.findtext("id")
        if ident and ident.isdecimal():item_names[int(ident)]="".join(item.find("name").itertext()).strip() if item.find("name") is not None else ""
    table=(ROOT/"mp_itemtable.js").read_text(encoding="utf-8-sig")
    matches=re.finditer(r'(\d+):"((?:\\.|[^"])*)"',table)
    mappings={}
    for m in matches:
        try:
            decoded=json.loads('"'+m[2]+'"')
            if re.fullmatch(r"t\d+",decoded):mappings[int(m[1])]=decoded
        except ValueError:continue
    OUT.mkdir(parents=True,exist_ok=True)
    # Store original object-type construction data, not only event actions.
    # For trees this includes default instance variables (hit endurance);
    # inspecting event actions alone cannot establish an initial value.
    source_objects = {}
    for object_id in (184, 207, 209, 210, 576, 577, 581, 348, 349):
        record = p[3][object_id]
        source_objects[str(object_id)] = [
            {"field": index,
             "value": value if index != 7 else
                {"animation_count": len(value) if isinstance(value, list) else None}}
            for index, value in enumerate(record)
        ]
    (OUT/"priority_world_object_defaults.json").write_text(
        json.dumps(source_objects, ensure_ascii=False, indent=2)+"\n",encoding="utf-8")
    print("ROOT_ORIGINAL_TREE_OBJECTS:", {
        key: [(field["field"],str(field["value"])[:150]) for field in row]
        for key,row in source_objects.items() if key in ("207","209")
    })
    with (OUT/"original_item_id_reference.csv").open("w",encoding="utf-8",newline="") as f:
        w=csv.writer(f);w.writerow(["item_id","source_english_name","mp_itemtable_sprite","construct_object_id"])
        for item_id in sorted(set(item_names)|set(mappings)):
            label=mappings.get(item_id,"")
            w.writerow([item_id,item_names.get(item_id,""),label,int(label[1:]) if label.startswith("t") else ""])
    hits=[]
    def traverse(events,sheet,anc=(),path=(),parent_nodes=()):
        for i,e in enumerate(events):
            if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
            uid=e[4];this=path+(i,)
            if uid in WORLD_UIDS:
                actions=e[6] if isinstance(e[6],list) else []
                sounds=[{"raw":a,"sound_literals":strings(a[5:])}
                        for a in actions if isinstance(a,list) and len(a)>5 and a[0]==187 and a[1] in (81,398)]
                drops=[{"raw":a,"args":a[5:]} for a in actions if isinstance(a,list)
                       and len(a)>5 and a[0]==189 and a[1]==69 and "Spawn_drop" in strings(a[5:])]
                changes=[a for a in actions if isinstance(a,list) and len(a)>1 and a[1] in (72,137,163)]
                hits.append({"label":WORLD_UIDS[uid],"sheet":sheet,"uid":uid,"path":this,
                             "parent_uids":anc,
                             "parent_events":[{"uid":p[4],"conditions":p[5],"actions":p[6]} for p in parent_nodes],
                             "conditions":e[5],"actions":actions,
                             "sounds":sounds,"drop_actions":drops,"visual_changes":changes,
                             "child_event_uids":[x[4] for x in (e[7] if len(e)>7 and isinstance(e[7],list) else []) if isinstance(x,list) and len(x)>4 and x[0]==0]})
            if len(e)>7 and isinstance(e[7],list):
                traverse(e[7],sheet,anc+(uid,),this,parent_nodes+(e,))
    for s in p[6]:traverse(s[1],s[0])
    (OUT/"priority_world_original_events.json").write_text(json.dumps(hits,ensure_ascii=False,indent=2)+"\n")
    lines=["# Priority original world event chains","",
           "Extracted directly from the root original (not the stale cache).",
           "The full original condition/action arrays are preserved in priority_world_original_events.json.","",
           "| Original chain | Event UID | Source record path | Direct conditions | Direct actions | Sounds | Drop call actions |",
           "|---|---:|---|---:|---:|---|---:|"]
    found={x["label"] for x in hits}
    for x in hits:
        names=sorted(set(n for s in x["sounds"] for n in s["sound_literals"]))
        lines.append("| "+x["label"]+" | "+str(x["uid"])+" | "+x["sheet"]+":"+".".join(map(str,x["path"]))+
                     " | "+str(len(x["conditions"]))+" | "+str(len(x["actions"]))+
                     " | "+", ".join(names)+" | "+str(len(x["drop_actions"]))+" |")
    missing=sorted(set(WORLD_UIDS.values())-found)
    lines+=["","## Original localized tree-drop item IDs","",
            "| Source item ID | English item name | MP table sprite ID |",
            "|---|---|---|"]
    for i in (37,38,50):
        lines.append("| "+str(i)+" | "+item_names.get(i,"(not in XML)")+" | "+mappings.get(i,"(not in table)")+" |")
    lines+=["","Missing priority UIDs from root: "+(", ".join(missing) if missing else "none"),"",
            "The item names above are LABELS from original XML; actual drops require original event conditions.",
            "Source UIDs may repeat; use sheet and the array index path from each event to select the intended chain.",
            "The exact sound parameters, dynamic item expressions and child event conditions remain preserved as raw JSON.",
            "Do not claim semantic parity until the original runtime ACE IDs are interpreted and gameplay verified."]
    (OUT/"priority_world_original_events.md").write_text("\n".join(lines)+"\n")
    # Bounded original event traces for human review in audit CI logs.
    for h in hits:
        if h["label"] not in ("tree_207_hit", "tree_207_fall",
                              "door_open", "door_close", "house_transparency_group"):
            continue
        print("SOURCE_CHAIN",json.dumps({
            "label":h["label"],"path":h["sheet"]+":"+".".join(map(str,h["path"])),
            "parents":[{"uid":p["uid"],"conditions":p["conditions"][:4],
                        "actions":p["actions"][:4]} for p in h["parent_events"][-4:]],
            "conditions":h["conditions"][:5],
            "actions":h["actions"][:10],
            "child_uids":h["child_event_uids"][:12]
        },separators=(",",":"),ensure_ascii=False)[:4000])
    print(json.dumps({"priority_events_found":len(hits),"missing":missing,
                      "item_name_count":len(item_names),"item_sprite_map_count":len(mappings),
                      "tree_drop_labels":{str(i):item_names.get(i) for i in (37,38,50)}},indent=2))
if __name__=="__main__":main()
