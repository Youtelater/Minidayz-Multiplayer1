#!/usr/bin/env python3
"""Source-first inventory of parity obligations for systems started in the 3DS port.

This is a static GAP AUDIT, not a proof of parity. It preserves original event
paths, inherited conditions, action arrays, function calls and audio references.
Never infer behavior solely from source labels or C++ string matches.
"""
import argparse
import csv
import hashlib
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

from audit_original_world_exhaustive import ROOT, decode, strings

# Ancestor group labels are authoritative source organization. A source event
# may belong to several workstreams; these are not mutually exclusive.
AREAS = {
    "world_generation": ("map generation", "create_map", "location loader", "buildings and obstacles", "bunker_generate", "bridge level", "level_change", "envir_effects"),
    "world_interactions": ("hitting_obstacles", "range_destroy", "range_less_power", "house_transparenting", "doors_auto", "obstacles_transparenting", "big_obstacles_transparenting", "merged pick and interact", "building mode", "gardening", "barberwire"),
    "items_loot_inventory": ("loot respawn", "spawn parameters", "spawn_drop", "picking player items", "picking inventory items", "droppin items", "pickpile", "loot_checker2", "stash", "airdrop", "inventory"),
    "survival_weather": ("time cyles and weather", "day&night", "rain", "sleeping", "fireplace params", "envir_effects", "disease", "hunger", "temperature", "thirst"),
    "crafting": ("craft", "building mode"),
    "player_equipment": ("player", "equipment", "backpack", "skins", "perks", "animation"),
    "combat_weapons": ("bullet", "shoot", "reload", "melee", "damage dealing", "molotov_damage", "beartrap", "hitting_obstacles"),
    "npc_zombies_animals": ("npcs", "zombies", "zeds_common", "z_spawners", "npc_manager", "spawn_npc", "bots", "bot manager", "animals", "wolf"),
    "vehicles": ("drivable vehicles", "vehicle collisions", "vehicle layering", "heli"),
    "menu_hud_quests": ("menu", "hud", "interface", "quests", "achievements", "tutorial", "map", "final_day"),
    "saving_progression": ("sav", "save", "load", "progress", "quest", "level_change", "final_day"),
    "audio_animation": ("sound", "audio", "music", "animation", "rain", "footstep", "hitting_obstacles", "doors_auto"),
}
# These native components are candidate locations to inspect; existence is NOT
# evidence their original event chains have been implemented.
NATIVE_FILES = {
    "world_generation": ("source/world.cpp", "source/location_event_runtime.cpp"),
    "world_interactions": ("source/main.cpp", "source/world.cpp", "include/minidayz/source_tree_fall_rules.hpp", "include/minidayz/source_door_auto.hpp", "include/minidayz/source_roof_interaction.hpp"),
    "items_loot_inventory": ("source/inventory.cpp", "source/main.cpp", "source/world.cpp"),
    "survival_weather": ("source/player.cpp", "source/main.cpp"),
    "crafting": ("source/crafting.cpp", "source/main.cpp"),
    "player_equipment": ("source/player.cpp", "source/main.cpp"),
    "combat_weapons": ("source/main.cpp", "source/player.cpp"),
    "npc_zombies_animals": ("source/npc_system.cpp", "source/main.cpp"),
    "vehicles": ("source/world.cpp", "source/main.cpp"),
    "menu_hud_quests": ("source/main.cpp",),
    "saving_progression": ("source/save_game.cpp", "source/world_visual_save.cpp", "source/main.cpp"),
    "audio_animation": ("source/stage2_audio.cpp", "source/main.cpp"),
}
IMPORTANT_UIDS = {
    6061166023614368: "door_open",
    243854400993751: "door_close",
    5249390718058362: "house_roof_opacity",
    5688415260272274: "automatic_doors",
    8754965976446553: "tree_207_hit",
    5386206472407495: "tree_209_hit",
    7227858680569601: "tree_184_hit",
    9193333425135665: "tree_576_hit",
    4761103342332918: "tree_577_hit",
    3531921662641626: "tree_581_hit",
    3772264929481614: "loot_respawn",
    5418835019314848: "spawn_drop",
    7952760861768009: "trigger_spawn",
}
AUDIO_ACTION = {(187, 81), (187, 398)}
VISUAL_CMDS = {72, 132, 137, 163}
def source_groups(event):
    flag = event[1] if len(event)>1 else None
    return flag[1].strip() if isinstance(flag,list) and len(flag)>1 and isinstance(flag[1],str) else ""

def classify(groups):
    labels = " / ".join(groups).lower()
    return [a for a,needles in AREAS.items() if any(token in labels for token in needles)]

def csvwrite(path, fields, rows):
    with path.open("w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=fields);w.writeheader();w.writerows(rows)

def scan(project):
    rows=[];audios=[];anchors={};totals=Counter()
    def visit(events,sheet,ancestor_uids=(),ancestry=(),source_path=()):
        for idx,e in enumerate(events):
            if not isinstance(e,list) or len(e)<7 or e[0]!=0:continue
            uid=e[4]
            g=source_groups(e)
            groups=ancestry+((g,) if g else ())
            areas=classify(groups)
            path=sheet+":"+".".join(map(str,source_path+(idx,)))
            cond=e[5] if isinstance(e[5],list) else []
            acts=e[6] if isinstance(e[6],list) else []
            totals['__visited']+=1
            totals['__actions']+=len(acts)
            children=e[7] if len(e)>7 and isinstance(e[7],list) else []
            sound=[]
            for index,a in enumerate(acts):
                if not isinstance(a,list) or len(a)<2: continue
                if tuple(a[:2]) in AUDIO_ACTION:
                    literals=strings(a[5:])
                    audios.append({"areas":"|".join(areas) or "unclassified",
                                   "sheet":sheet,"event_uid":uid,"path":path,
                                   "group_chain":"/".join(groups),"action_index":index,
                                   "sound_literals":"|".join(literals),
                                   "action_json":json.dumps(a,separators=(",",":"),ensure_ascii=False)})
                    sound.extend(literals)
            if areas:
                objects=sorted({a[0] for a in cond+acts if isinstance(a,list) and a and isinstance(a[0],int) and a[0]>=0})
                calls=[strings(a[5:]) for a in acts if isinstance(a,list) and len(a)>1 and tuple(a[:2])==(189,69)]
                rows.append({"areas":"|".join(areas),"sheet":sheet,"event_uid":uid,
                             "source_path":path,"group_chain":"/".join(groups),
                             "parent_uids":"|".join(map(str,ancestor_uids)),
                             "condition_count":len(cond),"action_count":len(acts),
                             "source_audio":"|".join(sound),
                             "source_functions":"|".join(n for group in calls for n in group),
                             "visual_actions":sum(1 for a in acts if isinstance(a,list) and len(a)>1 and a[1] in VISUAL_CMDS),
                             "object_ids":"|".join(map(str,objects)),
                             "conditions_json":json.dumps(cond,separators=(",",":"),ensure_ascii=False),
                             "actions_json":json.dumps(acts,separators=(",",":"),ensure_ascii=False)})
                for a in areas: totals[a]+=1
            if uid in IMPORTANT_UIDS:
                anchors.setdefault(str(uid),[]).append({"name":IMPORTANT_UIDS[uid],
                    "sheet":sheet,"source_path":path,"ancestor_uids":ancestor_uids,
                    "groups":groups,"conditions":cond,"actions":acts,
                    "children_count":len(children)})
            visit(children,sheet,ancestor_uids+(uid,),groups,source_path+(idx,))
    for sh in project[6]:visit(sh[1],sh[0])
    return rows,audios,anchors,totals

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--out",type=Path,default=ROOT/"audit-output/started-parity")
    opts=p.parse_args()
    opts.out.mkdir(parents=True,exist_ok=True)
    raw=(ROOT/"data.deobfuscated.js").read_bytes()
    project=decode(raw)
    rows,audios,anchors,counts=scan(project)
    print("SOURCE_SCAN_DIAGNOSTIC", "classified",len(rows),"audio",len(audios),"area_events",dict(counts))
    assert totals["__visited"]>20000 and totals["__actions"]>60000 and len(audios)>400, "Source scan unexpectedly lost event/audio coverage"
    native={}
    for area,paths in NATIVE_FILES.items():
        native[area]=[{"path":path,"exists":(ROOT/path).is_file()} for path in paths]
    md=["# Started systems — original MiniDAYZ vs Nintendo 3DS gap audit",
        "",
        "Source: **root `data.deobfuscated.js`** (not the stale source snapshot).",
        "Source SHA-256: `"+hashlib.sha256(raw).hexdigest()+"`.",
        "",
        "**WARNING:** This is an exhaustive *source-obligation index within the configured workstream groups*,",
        "not proof of parity. Native source-file existence and asset availability do not establish that",
        "actions actually fire; tests and Azahar behavior must be checked separately. Numeric ACE IDs",
        "are preserved rather than given invented names. Read inherited conditions and Construct",
        "picking rules in the original before resolving a mismatch.",
        "",
        "## Source obligations by workstream", "",
        "| Started system | Source event rows | Direct audio actions | Candidate native files (not parity proof) |",
        "|---|---:|---:|---|"]
    for area in AREAS:
        n=sum(area in r["areas"].split("|") for r in audios)
        md.append("| "+area.replace("_"," ").title()+" | "+str(counts[area])+" | "+str(n)+" | "+", ".join(p["path"].split("/")[-1] for p in native[area] if p["exists"])+" |")
    md.extend(["","An event may belong to several workstreams. All original audio actions, including",
               "unclassified actions, remain in `source_audio_actions.csv`. The complete",
               "condition/action arrays and original event paths are in `source_event_obligations.csv`.",
               "",
               "## Source event anchors needing full ancestor/action reconciliation",""])
    for uid,name in IMPORTANT_UIDS.items():
        records=anchors.get(str(uid),[])
        refs="; ".join(r["source_path"] for r in records[:3])
        md.append("- **"+name+"** UID `"+str(uid)+"`: "+(refs if refs else "**NOT FOUND — investigate**"))
    md.extend(["","## Confirmed and high-risk implementation gaps (source vs current static code)",""])
    main_file=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
    audio_file=(ROOT/"source/stage2_audio.cpp").read_text(encoding="utf-8")
    tree_file=(ROOT/"include/minidayz/source_tree_fall_rules.hpp").read_text(encoding="utf-8")
    has_tree_sound=bool(re.search(r"play_original_world_sound\s*\([^;]*(?:Chop|TreeFall)",main_file,re.S))
    door_calls=bool(re.search(r"play_original_world_sound\s*\(",main_file))
    has_tree_hit=bool(re.search(r"apply_source_tree_hit\s*\(",main_file))
    has_fall_executor="tree_fall_rule" in tree_file
    findings=[
        ("Tree hit and falling audio","MISSING LIVE DISPATCH" if not has_tree_sound else "REVIEW",
         "Source tree hit/fall events require chop/treefall sound. The audio files and playback method exist, but no tree-sound dispatch was found in live main.cpp." if not has_tree_sound else "Tree audio dispatch text exists; verify original conditions, selection, and actual sound playback."),
        ("Live tree hit input","MISSING LIVE DISPATCH" if not has_tree_hit else "REVIEW",
         "Tree fall rules exist; live main.cpp lacks an apply_source_tree_hit call. Inspect projectile 1031 and melee event chains before adding the hit hook."),
        ("Door sounds","PARTIAL IMPLEMENTATION" if door_calls else "MISSING LIVE DISPATCH",
         "The live game calls world-effect audio for doors; inspect source picking, 348 companion obstacle, collision polygon, intervals and per-sound branch guards."),
        ("Tree fall rewards","PARTIAL IMPLEMENTATION" if has_fall_executor else "REVIEW",
         "The source fall rewards are represented in native rules but end-to-end drop spawning, pickup, RNG ordering and saving require gameplay checks."),
        ("Roofs and interiors","PARTIAL IMPLEMENTATION",
         "Opacity and family membership are wired; verify inherited 1048/346 overlap conditions, layer ordering, shadows and visibility in Azahar."),
        ("Location loot markers","SOURCE-PARITY BLOCKER",
         "Existing fixed-seed world tests reported zero eligible category-15 loot markers. The original map-location creation/loot event chain still needs a live trace."),
        ("Dynamic world state saving","PARTIAL IMPLEMENTATION",
         "The visual-state journal exists; original per-object HP, collision, door companion, destroyed object, drops and respawn state are not all persisted."),
        ("Asset/audio completeness","OPEN AUDIT",
         "Source audit indexes 461 audio actions and 14,505 frame references; reachability/dispatch for each effect and unreferenced PNG investigation remain open."),
    ]
    for name,status,detail in findings: md.append("- **"+name+" — "+status+".** "+detail)
    md.extend(["","## Verification protocol","",
        "For every marked-original event: review event and ancestor conditions in the CSV,",
        "translate the action sequence (including any dynamic function call), map each audio",
        "and sprite frame reference, record native implementation commit and focused test,",
        "and only then check Azahar/real-3DS playback and persistence.",
        "",
        "This artifact is an audit finding, **not a certification that every game feature is finished**.",
        ""])
    opts.out.joinpath("ORIGINAL_VS_NATIVE_PARITY.md").write_text("\n".join(md),encoding="utf-8")
    csvwrite(opts.out/"source_event_obligations.csv",list(rows[0]),rows)
    csvwrite(opts.out/"source_audio_actions.csv",list(audios[0]),audios)
    opts.out.joinpath("source_anchor_chains.json").write_text(json.dumps(anchors,indent=2,ensure_ascii=False),encoding="utf-8")
    result={"source_sha256":hashlib.sha256(raw).hexdigest(),"source_events_classified":len(rows),
        "source_audio_actions":len(audios),"source_audio_unclassified":sum(x["areas"]=="unclassified" for x in audios),
        "workstream_event_counts":dict(counts),"findings":[{"area":a,"status":b,"description":c} for a,b,c in findings],
        "anchors_found":{k:len(v) for k,v in anchors.items()},"native_candidate_files":native}
    opts.out.joinpath("summary.json").write_text(json.dumps(result,indent=2),encoding="utf-8")
    print("ROOT_SHA",result["source_sha256"])
    print("CLASSIFIED_EVENTS",len(rows),"SOURCE_AUDIO_ACTIONS",len(audios),"AUDIO_OUTSIDE_GROUPS",result["source_audio_unclassified"])
    for area in AREAS:print("WORKSTREAM",area,"EVENTS",counts[area],"AUDIO_ACTIONS",sum(area in x["areas"].split("|") for x in audios))
    for name,status,_ in findings: print("PARITY_FINDING",name,status)
    print("ANCHOR_UIDS",len(anchors),"OF",len(IMPORTANT_UIDS))
    print("AUDIT_ARTIFACT",opts.out)
if __name__=="__main__": main()
