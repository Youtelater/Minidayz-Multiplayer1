#!/usr/bin/env python3
"""Generate source-authored initial building loot categories, chance and items.

This uses ONLY root data.deobfuscated.js extracted Construct Game_events.
Every normal single-player Trigger_spawn category and exact weighted choose()
argument is retained, including duplicate item IDs, no-drop ID 0 and
category 23's explicit Else. Unsupported/changed source actions fail closed.
"""
from pathlib import Path
from collections import Counter
import json,re,xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/"data/stage1/construct_project_parsed.json"
HEADER=ROOT/"include/minidayz/generated/original_initial_building_loot.hpp"
raw=SOURCE.read_text(encoding="utf-8-sig")
project=json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)","-",raw))["project"]
game=next(sh[1] for sh in project[6] if sh[0]=="Game_events")
known={int(i.findtext("id")) for i in ET.parse(ROOT/"l_eng_items.xml").getroot().findall("item")}
def walk(events,parents=()):
    for event in events if isinstance(events,list) else []:
        if not isinstance(event,list) or len(event)<7 or event[0]!=0:continue
        yield event,parents
        if len(event)>7 and isinstance(event[7],list):
            yield from walk(event[7],parents+(event,))
def find(uid):
    entries=[e for e,_ in walk(game) if e[4]==uid]
    if len(entries)!=1:raise SystemExit(f"Root event UID {uid} not unique ({len(entries)})")
    return entries[0]
def literal(expr):
    if isinstance(expr,list) and len(expr)==2 and expr[0]==0 and isinstance(expr[1],int):
        return [expr[1]]
    if isinstance(expr,list) and len(expr)==3 and expr[:2]==[19,82]:
        result=[x[1] for x in expr[2]
            if isinstance(x,list) and len(x)==2 and x[0]==0 and isinstance(x[1],int)]
        if len(result)!=len(expr[2]) or not result:raise SystemExit("Unrecognized literal choose args")
        return result
    raise SystemExit("Unexpected original Spawn_drop selection expression "+str(expr)[:150])
def is_spawn_drop(a):
    return isinstance(a,list) and len(a)>5 and a[:2]==[189,69] and (
        isinstance(a[5],list) and len(a[5])==2 and a[5][0]==[1,[2,"Spawn_drop"]])
def is_trigger(a):
    return (isinstance(a,list) and len(a)>5 and a[:2]==[189,69] and
        isinstance(a[5],list) and a[5] and a[5][0]==[1,[2,"Trigger_spawn"]])
def category_condition(c):
    if not (isinstance(c,list) and len(c)>9 and c[:2]==[218,71]):return None
    ps=c[9]
    if len(ps)!=3 or ps[:2]!=[[10,0],[8,0]]:raise SystemExit("Unknown loot category comparison")
    value=literal(ps[2][1]) if isinstance(ps[2],list) and ps[2][0]==7 else []
    if len(value)!=1 or not 0<value[0]<65536:raise SystemExit("Unknown original loot marker category")
    return value[0]
def parse_gate(e):
    gates=[]
    for c in e[5]:
        if c[:2]==[-1,70]:
            if len(e[5])!=1:raise SystemExit("Unexpected source Else with extra condition")
            gates.append((3,[],0))
            continue
        if c[:2]!=[-1,51]:raise SystemExit("Unknown loot child predicate "+str(c)[:200])
        if len(c[9])!=3 or c[9][1] not in ([8,0],[8,1]) or c[9][2][0]!=7:
            raise SystemExit("Unknown loot chance operator "+str(c[9]))
        lhs=c[9][0][1]
        rhs=literal(c[9][2][1])
        if len(rhs)!=1:raise SystemExit("Unknown source chance comparator")
        # Original category17 uses round(random(1,100))==55, NOT
        # choose(1,...,100). Capture this exact AST separately.
        if lhs==[19,80,[[19,95,[[0,1],[0,100]]]]]:
            if c[9][1]!=[8,0]:raise SystemExit("Unexpected round(random()) comparison")
            gates.append((4,[],rhs[0]))
        else:
            vals=literal(lhs)
            gates.append((1 if c[9][1]==[8,0] else 2,vals,rhs[0]))
    if len(gates)>1:raise SystemExit("Compound random gate not independently ported")
    return gates[0] if gates else (0,[],0)

trigger=find(7952760861768009)
if len(trigger[7])!=2 or trigger[7][0][4]!=550456525990174 or (
        trigger[7][1][4]!=289942360829320 or
        trigger[7][1][5][0][:2]!=[-1,70]):
    raise SystemExit("Original SEED_pubg vs normal Trigger_spawn branches changed")
normal=trigger[7][1]
if len(normal[7])!=1:
    raise SystemExit("Original non-PUBG source marker selection shape changed")
picked=normal[7][0]
if not any(c[:2]==[218,92] for c in picked[5]) or not any(
    c[:2]==[218,108] and c[5] is True and c[9]==[[4,995]]
    for c in picked[5]):
    raise SystemExit("Original Source Trigger_spawn UID/family995 guards changed")

rows=[]
flat_choices=[]
flat_gates=[]
for category_event in picked[7]:
    if not isinstance(category_event,list) or len(category_event)<7 or category_event[0]!=0:
        raise SystemExit("Unrecognized source category event")
    # Final original 218.Destroy after the category branches is a separate
    # one-shot marker cleanup action, not a 49th loot category.
    if category_event[4]==723893136813997:
        if category_event[5] or len(category_event[6])!=1 or (
                category_event[6][0][:2]!=[218,53]):
            raise SystemExit("Original post-Trigger_spawn marker cleanup changed")
        continue
    conditions=[category_condition(c) for c in category_event[5]]
    conditions=[c for c in conditions if c is not None]
    if len(conditions)!=1 or len(category_event[5])!=1:
        raise SystemExit("Unrecognized normal loot category conditions "+str(category_event[4]))
    category=conditions[0]
    leaves=list(walk([category_event]))
    found=0
    for leaf,anc in leaves:
        calls=[a for a in leaf[6] if is_spawn_drop(a)]
        if not calls:continue
        if len(calls)!=1:raise SystemExit("More than one original loot drop at one category leaf")
        gate_path=[*anc,leaf][1:]
        if len(gate_path)>1:raise SystemExit("Nested source loot guards beyond simple probability")
        gate=parse_gate(gate_path[0]) if gate_path else (0,[],0)
        a=calls[0]
        params=a[5][1]
        if not isinstance(params,list) or len(params)!=7 or params[0]!=13:
            raise SystemExit("Unexpected source Spawn_drop argument count")
        expected_xy=[
            [7,[19,80,[[20,218,65,False,None]]]],
            [7,[19,80,[[20,218,66,False,None]]]]
        ]
        if params[2:4]!=expected_xy or params[4:]!=[[7,[0,-1]]]*3:
            raise SystemExit("Original Spawn_drop coordinates or suffix changed")
        if not isinstance(params[1],list) or params[1][0]!=7:
            raise SystemExit("Unexpected source Spawn_drop item expression wrapper")
        items=literal(params[1][1])
        if any(item not in known and item!=0 for item in items):
            raise SystemExit(f"Unknown source item ID in category {category}")
        if len(flat_choices)+len(items)>65535 or len(flat_gates)+len(gate[1])>65535:
            raise SystemExit("Source initial loot table exceeds 16-bit offsets")
        rows.append((category,leaf[4],a[3],len(flat_choices),len(items),
                     len(flat_gates),len(gate[1]),gate[2],gate[0]))
        flat_choices.extend(items)
        flat_gates.extend(gate[1])
        found+=1
    # Original marker category25 has no Spawn_drop in normal mode.
    # Its event is source-authored but deliberately drops nothing.
    if not found:
        if category==25:continue
        raise SystemExit("Unexpected source category with no Spawn_drop: "+str(category))
    if found>2:raise SystemExit("Source category has more than two Spawn_drop branches")
    if found==2 and not (rows[-2][8] in (1,2) and rows[-1][8]==3):
        raise SystemExit("Source multiple category branches lack matching chance/Else")
if len(rows)!=49 or len({r[0] for r in rows})!=48:
    raise SystemExit(f"Original normal loot category coverage changed: {len(rows)}")
for required in (1,2,9,15,19,23,44,101,102,103,104,105):
    if required not in (r[0] for r in rows):
        raise SystemExit(f"Original required category {required} missing")

# Root Spawn_drop function 5418835019314848 does not directly create
# every item ID. Four achievement-locked items have exact alternate item
# creation in an Else branch; 351 has no source branch and must not appear.
# Map both cases from the authoritative root event and the original MP
# object lookup, never guess another item's texture or ID.
spawn_function=find(5418835019314848)
menu=next(sh[1] for sh in project[6] if sh[0]=="Menu_Events")
original_item_map=(ROOT/"mp_itemtable.js").read_text(encoding="utf-8-sig")
original_item_map=re.sub(r"\\\\u([0-9a-fA-F]{4})",
  lambda m:chr(int(m.group(1),16)),original_item_map)
start=original_item_map.find("MDZItemTable")
first=original_item_map.find("{",start)
last=original_item_map.find("}",first)
if start<0 or first<0 or last<0:raise SystemExit("Missing root multiplayer sprite table")
object_to_item={}
for match in re.finditer(r'(\\d+)\\s*:\\s*"(t\\d+)"',original_item_map[first+1:last]):
    original_item_id=int(match.group(1))
    original_object_name=match.group(2)
    original_oid=int(original_object_name[1:])
    if original_oid in object_to_item:raise SystemExit("Multiple item IDs for original sprite")
    object_to_item[original_oid]=original_item_id

def direct_created_object(event):
    source=[a[5][0][1] for a in event[6] if isinstance(a,list) and len(a)>5
        and a[:2]==[-1,94] and a[5] and a[5][0][0]==4]
    return source[0] if len(source)==1 else None

direct_ids=set()
special=[]
for event in spawn_function[7]:
    if not isinstance(event,list) or len(event)<7 or event[0]!=0:continue
    conditions=[c for c in event[5] if c[:2]==[189,112] and len(c)>9]
    if len(conditions)!=1 or not conditions[0][9][2]==[7,[0,conditions[0][9][2][1][1]]]:
        continue
    requested=int(conditions[0][9][2][1][1])
    object_id=direct_created_object(event)
    if object_id is not None:
        direct_ids.add(requested)
        continue
    if requested not in (118,119,124,125):continue
    children=event[7] if len(event)>7 else []
    if len(children)!=2:raise SystemExit(f"Original locked item {requested} source changed")
    first_branch,else_branch=children
    predicates=first_branch[5]
    if len(predicates)!=1 or predicates[0][:2]!=[-1,46] or (
       predicates[0][9][1]!=[8,0] or predicates[0][9][2]!=[7,[0,1]]):
        raise SystemExit("Original special loot unlock guard changed")
    unlock=predicates[0][9][0][1]
    if unlock not in ("item_unlock_plast","item_unlock_smoke",
                      "item_unlock_laser","item_unlock_lighter"):
        raise SystemExit("Unknown source unlock flag")
    if else_branch[5][0][:2]!=[-1,70]:
        raise SystemExit("Missing original special loot Else branch")
    unlocked_object=direct_created_object(first_branch)
    fallback_object=direct_created_object(else_branch)
    fallback_item=object_to_item.get(fallback_object)
    if not unlocked_object or not fallback_item:
        raise SystemExit(f"Missing original locked loot fallback for item {requested}")
    defaults=[v for v in menu if isinstance(v,list) and len(v)>2 and
              v[0]==1 and v[1]==unlock]
    if len(defaults)!=1 or defaults[0][2]!=0:
        raise SystemExit(f"Original unlock flag {unlock} not default 0")
    special.append((requested,fallback_item,unlocked_object,unlock))
if len(special)!=4:
    raise SystemExit("Original achievement-lock loot fallback coverage changed")
missing=set(flat_choices)-direct_ids-{r[0] for r in special}
if missing!={351} or 351 in direct_ids:
    raise SystemExit(f"Original unsupported initial Spawn_drop IDs changed: {missing}")

# Root create_map_element marker spawn event UIDs with an in-event call
# to Trigger_spawn, used only for the *non-house-marker* creation pathways.
# House marker actions are already tracked individually, including the two
# locations where the original did NOT call Trigger_spawn.
source_func=7736925111997576
source_events=[]
for e,parents in walk(game):
    if not any(p[4]==source_func for p in parents):continue
    for i,a in enumerate(e[6]):
        if not(isinstance(a,list) and len(a)>5 and a[1] in (73,94) and
            a[5] and a[5][0]==[4,218]):continue
        following=e[6][i+1:]
        for j,nxt in enumerate(following):
            if isinstance(nxt,list) and len(nxt)>5 and nxt[1] in (73,94) and (
                    nxt[5] and nxt[5][0]==[4,218]):
                following=following[:j]
                break
        sets=[x for x in following if isinstance(x,list) and len(x)>5 and
              x[:2]==[218,88] and x[5][0]==[10,0]]
        category=None
        if sets and len(sets[0][5])>1 and sets[0][5][1][0]==7:
            try:
                values=literal(sets[0][5][1][1])
                if len(values)==1:category=values[0]
            except SystemExit:
                pass
        source_events.append((e[4],category,any(is_trigger(x) for x in following)))
if len(source_events)<160:raise SystemExit("Source building marker creation coverage shrank")
# These are conservative event-level tags for native Mapgen/companion creations.
# Do not tag authored root Map editor markers or markers without an original call.
trigger_event_uids=sorted({uid for uid,cat,hit in source_events if hit and cat is not None})
consistent_event_categories={}
for uid,cat,hit in source_events:
    if cat is None:continue
    if uid in consistent_event_categories and consistent_event_categories[uid]!=cat:
        consistent_event_categories[uid]=None
    elif uid not in consistent_event_categories:
        consistent_event_categories[uid]=cat
source_event_category_rows=sorted((uid,cat) for uid,cat in consistent_event_categories.items()
                                  if cat is not None)

def cpp(data,group):
    out=[]
    for i in range(0,len(data),group):out.append("    "+", ".join(map(str,data[i:i+group]))+",")
    return out or ["    0,"]
lines=["#pragma once","#include <cstddef>","#include <cstdint>",
       "namespace minidayz::items {",
       "struct SourceInitialBuildingLootRule {",
       " std::uint16_t category;std::uint64_t event_uid,action_uid;",
       " std::uint16_t first,choices,gate_first,gate_choices;",
       " std::int16_t gate_target;std::uint8_t gate_kind;",
       "};",
       "// gate_kind 0=always,1=choose==N,2=choose!=N,3=Else,4=round(random(1,100))==N.",
       f"inline constexpr std::size_t kSourceInitialBuildingLootRuleCount={len(rows)};",
       f"inline constexpr std::size_t kSourceInitialBuildingLootItemCount={len(flat_choices)};",
       "inline constexpr std::uint16_t kSourceInitialBuildingLootItems[]={"]
lines+=cpp(flat_choices,20)
lines+=["};",
        f"inline constexpr std::size_t kSourceInitialBuildingLootGateValueCount={len(flat_gates)};",
        "inline constexpr std::uint16_t kSourceInitialBuildingLootGateValues[]={"]
lines+=cpp(flat_gates,24)
lines+=["};","inline constexpr SourceInitialBuildingLootRule kSourceInitialBuildingLootRules[]={"]
for row in sorted(rows,key=lambda r:r[0]):
    category,event_uid,action_uid,first,count,gfirst,gcount,target,gkind=row
    lines.append(f" {{{category},{event_uid}ULL,{action_uid}ULL,{first},{count},{gfirst},{gcount},{target},{gkind}}},")
lines+=["};",
        f"inline constexpr std::size_t kSourceInitialMarkerCreationEventCount={len(trigger_event_uids)};",
        "inline constexpr std::uint64_t kSourceInitialMarkerCreationEventUIDs[]={"]
lines+=["    "+", ".join(str(uid)+"ULL" for uid in trigger_event_uids[i:i+6])+","
        for i in range(0,len(trigger_event_uids),6)]
lines+=["};",
        "struct SourceInitialMarkerCategoryEvent {std::uint64_t event_uid;std::uint16_t category;};",
        f"inline constexpr std::size_t kSourceInitialMarkerCategoryEventCount={len(source_event_category_rows)};",
        "inline constexpr SourceInitialMarkerCategoryEvent kSourceInitialMarkerCategoryEvents[]={"]
for uid,category in source_event_category_rows:
    lines.append(f" {{{uid}ULL,{category}}},")
lines+=["};",
        "struct SourceInitialSpecialItemReplacement {",
        " std::uint16_t requested_id,fallback_item,unlocked_object;",
        "};",
        f"inline constexpr std::size_t kSourceInitialSpecialItemCount={len(special)};",
        "inline constexpr SourceInitialSpecialItemReplacement kSourceInitialSpecialItems[]={"]
for request,fallback,unlocked,_ in sorted(special):
    lines.append(f" {{{request},{fallback},{unlocked}}},")
lines+=["};",
        "inline bool source_initial_locked_fallback(std::uint16_t id,",
        " std::uint16_t& fallback) noexcept {",
        " for(const auto& rule:kSourceInitialSpecialItems)",
        "  if(rule.requested_id==id){fallback=rule.fallback_item;return true;}",
        " return false;}",
        "inline constexpr std::uint16_t kSourceUnspawnableItem=351;",
        "inline bool source_initial_marker_event_category(std::uint64_t uid,",
        " std::uint16_t& category) noexcept {",
        " for(const auto& value:kSourceInitialMarkerCategoryEvents)",
        "  if(value.event_uid==uid){category=value.category;return true;}",
        " return false;}",
        "inline bool source_event_triggers_initial_loot(std::uint64_t uid) noexcept {",
        " for(const auto value:kSourceInitialMarkerCreationEventUIDs)if(value==uid)return true;",
        " return false;}","} // namespace minidayz::items",""]
HEADER.parent.mkdir(parents=True,exist_ok=True)
HEADER.write_text("\n".join(lines),encoding="utf-8")
print("SOURCE ORIGINAL INITIAL LOOT",len(rows),"branches",
      len({r[0] for r in rows}),"categories",len(flat_choices),"weighted item choices",
      len(flat_gates),"chance values",len(trigger_event_uids),"source marker-creation event UIDs")
print("SOURCE CHANCE EXAMPLE: house9",[(r[0],r[7],r[8]) for r in rows if r[0]==9])
print("SOURCE ROOT SPAWN_DROP UNLOCKED DEFAULT FALLBACKS",special,"unspawnable",sorted(missing))
