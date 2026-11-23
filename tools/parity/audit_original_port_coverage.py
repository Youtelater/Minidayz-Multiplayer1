#!/usr/bin/env python3
"""Exact source -> 3DS parity register. Never infer "implemented" from an asset.

The key is (event-sheet ordinal, child-index path), because Construct repeats
some UIDs. A source event is VERIFIED only when an explicit, reviewed
implementation mapping + deterministic test names this exact source event and
matches its fingerprint. All other events are openly NOT VERIFIED.

All source expressions and inherited conditions are retained byte-for-byte
as decoded AST, including 7-element leaf nodes and repeated UID occurrences.
"""
from pathlib import Path
from collections import Counter
import hashlib,json,sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
source=ROOT/"data.deobfuscated.js"
original=decode(source.read_bytes())
out=ROOT/"audit-output/source-parity"
out.mkdir(parents=True,exist_ok=True)
registry=json.loads((ROOT/"data/parity/verified_original_events.json").read_text(encoding="utf-8"))
assert registry["schema_version"]==1
verified=registry["verified"]
assert isinstance(verified,list)
by_key={r["key"]:r for r in verified}
assert len(by_key)==len(verified),"Duplicate manually verified source occurrence"

def sha(obj):
 return hashlib.sha256(json.dumps(obj,ensure_ascii=False,
       separators=(",",":"),sort_keys=True).encode("utf-8")).hexdigest()
def event_children(event):
 return event[7] if len(event)>7 and isinstance(event[7],list) else []
def traverse(seq,sheet_id,sheet_name,index=(),parents=(),groups=(),inherited=()):
 for pos,e in enumerate(seq):
  if not isinstance(e,list) or not e or e[0]!=0:continue
  if len(e)<7:raise ValueError((sheet_name,index,pos,"truncated Construct event"))
  key=f"{sheet_id}:"+"/".join(map(str,(*index,pos)))
  gp=e[1][1] if isinstance(e[1],list) and len(e[1])>1 and isinstance(e[1][1],str) else None
  chain=(*groups,gp) if gp else groups
  conditions=e[5] if isinstance(e[5],list) else []
  actions=e[6] if isinstance(e[6],list) else []
  fingerprint=sha(e[:7])
  record=by_key.get(key)
  if record:
   assert record["source_fingerprint"]==fingerprint,(key,"original event changed")
   for prop in ("implementation","regression_test","scope"):
    assert record.get(prop),(key,"verification requires "+prop)
   for prop in ("implementation","regression_test"):
    assert (ROOT/record[prop]).is_file(),(key,"missing "+prop)
  yield {"key":key,"sheet":sheet_name,"sheet_ordinal":sheet_id,
    "uid":e[4],"event_source_length":len(e),"path":[*index,pos],
    "parent_uids":list(parents),"groups":list(chain),
    "inherited_conditions":list(inherited),"conditions":conditions,
    "actions":actions,"fingerprint":fingerprint,
    "verification":"reviewed_mapping" if record else "not_event_verified",
    "implementation":record["implementation"] if record else None,
    "regression_test":record["regression_test"] if record else None}
  yield from traverse(event_children(e),sheet_id,sheet_name,(*index,pos),
                      (*parents,e[4]),chain,(*inherited,*conditions))

def chosen_ids(action):
 # Original Construct choose(...) AST 19,82; retain repeated choices as
 # weights. Do not normalize, deduplicate, or invent probabilities.
 def visit(x):
  if isinstance(x,list):
   if len(x)>=3 and x[0:2]==[19,82] and isinstance(x[2],list):
    return [v[1] for v in x[2] if isinstance(v,list) and
            len(v)==2 and v[0]==0 and isinstance(v[1],int)]
   for child in x:
    result=visit(child)
    if result is not None:return result
  return None
 return visit(action[5])
def car_item_expression(action):
 # Second Spawn_drop argument is an expression for the item ID. Some
 # original tutorial/variant branches pass a single literal, not choose.
 item=action[5][1] if len(action[5])>1 else None
 options=chosen_ids(action)
 if options is not None:return "original_choose",options,item
 def single_literal(x):
  if isinstance(x,list):
   if len(x)==2 and x[0]==0 and isinstance(x[1],int):return x[1]
   if len(x)==2 and x[0] in (7,13):return single_literal(x[1])
  return None
 literal=single_literal(item)
 if literal is not None:return "original_literal",[literal],item
 # Never coerce a dynamic source expression into an invented item ID.
 return "original_dynamic_expression",[],item
def car_branch(record):
 ancestor=set(record["parent_uids"])
 return bool({7711324247664447,1792234758292318}&ancestor)
stats=Counter();sheets=Counter();car_entries=[];unverified_by_group=Counter()
with (out/"every_original_event.jsonl").open("w",encoding="utf-8") as dest:
 for sheet_idx,(sheet_name,events) in enumerate(original[6]):
  for row in traverse(events,sheet_idx,sheet_name):
   stats["events"]+=1;sheets[sheet_name]+=1
   stats["conditions"]+=len(row["conditions"])
   stats["actions"]+=len(row["actions"])
   if row["event_source_length"]==7:stats["seven_field_leaf_events"]+=1
   if row["verification"]=="reviewed_mapping":stats["reviewed_event_mappings"]+=1
   else:unverified_by_group[row["groups"][-1] if row["groups"] else "(none)"]+=1
   dest.write(json.dumps(row,ensure_ascii=False,separators=(",",":"))+"\n")
   for action in row["actions"]:
    if not isinstance(action,list) or len(action)<6 or action[:2]!=[189,69]:
     continue
    if not action[5] or action[5][0]!=[1,[2,"Spawn_drop"]]:
     continue
    if not car_branch(row):continue
    choice_kind,options,item_expr=car_item_expression(action)
    car_entries.append({"key":row["key"],"event_uid":row["uid"],
      "sheet":row["sheet"],"path":row["path"],"ancestor_uids":row["parent_uids"],
      "original_inherited_conditions":row["inherited_conditions"],
      "original_conditions":row["conditions"],
      "source_action":action,"original_item_expression":item_expr,
      "item_expression_kind":choice_kind,
      "item_choices_weighted":options,
      "source_item_choice_count":len(options),
      "verification":row["verification"],
      "native_car_interaction_implemented":False})

assert stats["events"]==22343,stats
assert stats["seven_field_leaf_events"]==14560,stats
assert stats["reviewed_event_mappings"]==len(verified),stats
assert len(car_entries)>=8,len(car_entries)
assert any(x["event_uid"]==226004896143721 for x in car_entries)
assert all(x["item_expression_kind"] in
    ("original_choose","original_literal","original_dynamic_expression")
    for x in car_entries)
cars={"authority":"root data.deobfuscated.js; original object 241",
      "warning":"Source trace only; car interaction/drop is NOT wired to the 3DS runtime.",
      "source_event_occurrences":len(car_entries),"branches":car_entries}
(out/"original_car_loot_event_manifest.json").write_text(
 json.dumps(cars,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
# Full action-occasion ledger: a verified event does not automatically
# prove each of its original action side-effects is implemented.
# Index nested ordinal path + action index, NEVER a UID alone (UIDs repeat).
import gzip
action_count=0
action_keys=set()
with (out/"every_original_event.jsonl").open("r",encoding="utf-8") as source_events:
 with gzip.open(out/"every_original_action.jsonl.gz","wt",encoding="utf-8") as sink:
  for raw in source_events:
   event=json.loads(raw)
   for action_index,action in enumerate(event["actions"]):
    action_key=event["key"]+"#"+str(action_index)
    if action_key in action_keys:
     raise AssertionError("Duplicate original action occurrence "+action_key)
    action_keys.add(action_key)
    action_count+=1
    sink.write(json.dumps({
      "action_key":action_key,"event_key":event["key"],
      "event_uid":event["uid"],"sheet":event["sheet"],
      "original_event_path":event["path"],
      "action_index":action_index,
      "action_uid":action[3] if isinstance(action,list) and len(action)>3 else None,
      "original_action":action,
      "inherited_conditions":event["inherited_conditions"],
      "own_conditions":event["conditions"],
      "parent_event_uids":event["parent_uids"],
      "group_path":event["groups"],
      "verification":event["verification"],
      "native_action_parity":"not_individually_verified"
    },separators=(",",":"),ensure_ascii=False)+"\n")
if action_count!=stats["actions"] or action_count!=63289:
 raise AssertionError(f"Original action occurrence loss {action_count}/{stats['actions']}")
print("FULL_ORIGINAL_ACTION_LEDGER",action_count,"individually indexed source actions")

summary={
 "source":"root data.deobfuscated.js",
 "source_sha256":hashlib.sha256(source.read_bytes()).hexdigest(),
 "status_semantics":"A decoded event is NOT implemented or tested without explicit mapping.",
 "counts":dict(stats),"action_occurrences_indexed":action_count,
 "car_spawn_drop_event_occurrences":len(car_entries),
 "car_system_3ds_implementation_verified":False,
 "unverified_groups":dict(unverified_by_group.most_common()),
 "sheet_event_counts":dict(sheets),
 "complete_original_port_verified":len(verified)==stats["events"]
}
(out/"parity_summary.json").write_text(
 json.dumps(summary,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
print("ORIGINAL_PARITY_SUMMARY",json.dumps({
 "total_events":stats["events"],
 "seven_field_events":stats["seven_field_leaf_events"],
 "reviewed_event_mappings":stats["reviewed_event_mappings"],
 "car_loot_branches":len(car_entries),
 "all_ported_and_verified":summary["complete_original_port_verified"]}))
