#!/usr/bin/env python3
"""Statically inventory root mp_*.js MOD scripts without running browser code.

The files are multiplayer add-on scripts from the source package, not
substitutes for Construct's data.deobfuscated.js / c2runtime.js. Generate
reports with exact source file hashes and decoded window symbol references;
do not execute or modify their scripts as part of single-player 3DS builds.
"""
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "data/stage1/mp_script_manifest.json"
EXPECTED = {
 "mp_authority.js":"Host-authoritative world entities, drop replication and spawn arbitration",
 "mp_entities.js":"Networked zombie/NPC entity state, hits, noise and damage exchange",
 "mp_events.js":"Multiplayer event replication for shots, reloads and combat animations",
 "mp_hostpanel.js":"Multiplayer host setup, difficulty/mode toggles, lobby configuration",
 "mp_interactions.js":"Replication of door and bush interactions and object state",
 "mp_itemtable.js":"Numeric item ID to Construct object/sprite table and item variants",
 "mp_join.js":"Joining and verifying a host's Construct world snapshot",
 "mp_joingate.js":"Join/load gating, world verification and recovery UI",
 "mp_kit.js":"Multiplayer loadout configuration, starting and respawn equipment",
 "mp_lang.js":"Eight-language translations for the multiplayer HUD and menus",
 "mp_menu.js":"Browser DOM multiplayer menu; overlays original Construct menu",
 "mp_overlay.js":"Browser DOM multiplayer world loading and disconnect overlays",
 "mp_players.js":"Multiplayer players, skins/gear visuals, checkpoints and state",
 "mp_playerstore.js":"Browser IndexedDB/localStorage multiplayer player checkpoints",
 "mp_props.js":"Networked props and Construct object property state",
 "mp_runtime_compat.js":"Helpers for obfuscated Construct runtime types/tilemaps/serialization",
 "mp_structures.js":"Replication of placed structures, events and ownership",
 "mp_throwables.js":"Host/network replication of throwable objects",
 "mp_transport.js":"Peer and relay transport abstraction for multiplayer rooms",
 "mp_worldseed.js":"Multiplayer seeded-world instrumentation and deterministic map checks",
 "mp_worldstate.js":"Host world-state synchronization and acknowledgements",
}
SINGLEPLAYER_REFERENCE = {
 "mp_itemtable.js", "mp_runtime_compat.js", "mp_worldseed.js",
 "mp_menu.js", "mp_players.js", "mp_events.js",
}
def decode_escaped_unicode(source):
    # Content is obfuscated JS; decoding string escapes for a static index
    # is NOT evaluating the JavaScript or following any network operations.
    return re.sub(r"\\u([0-9a-fA-F]{4})",
                  lambda m: chr(int(m.group(1), 16)), source)

def scan():
    all_paths=sorted(ROOT.glob("mp_*.js"))
    found={p.name for p in all_paths}
    expected=set(EXPECTED)
    if found!=expected:
        raise SystemExit(f"Unexpected root multiplayer scripts: missing={sorted(expected-found)}, extra={sorted(found-expected)}")
    html=(ROOT/"index.html").read_text(encoding="utf-8")
    if "test multiplayer version" not in html:
        raise SystemExit("index.html no longer identifies the multiplayer mod; review provenance")
    load_order=re.findall(r'<script[^>]*src="(mp_[^"]+\.js)"',html)
    if len(load_order)!=len(EXPECTED) or set(load_order)!=expected:
        raise SystemExit("Multiplayer load order/coverage differs from root mp_ files")
    rows=[]
    for path in all_paths:
        data=path.read_bytes()
        mirror=ROOT/"Minidayz-Multiplayer-main"/path.name
        same=mirror.exists() and mirror.read_bytes()==data
        src=decode_escaped_unicode(data.decode("utf-8-sig"))
        window_refs=sorted(set(re.findall(r'window\["([A-Za-z_$][\w$]{2,79})"\]',src)))
        symbols=[ref for ref in window_refs if ref.startswith(("MDZ","MP","mdz"))]
        rows.append({
            "file":path.name,
            "bytes":len(data),
            "sha256":hashlib.sha256(data).hexdigest(),
            "mirrored_identically":same,
            "load_order":load_order.index(path.name)+1,
            "role":EXPECTED[path.name],
            "integration":"source reference only (not vanilla runtime)" if path.name in SINGLEPLAYER_REFERENCE else "defer until actual multiplayer port",
            "symbols":symbols,
            "requires_browser_dom":"document" in src or "localStorage" in src or "sessionStorage" in src,
            "references_construct_runtime":"cr_getC2Runtime" in src,
            "references_peer":"Peer" in src or "peerjs" in src.lower(),
        })
    if not all(row["mirrored_identically"] for row in rows):
        raise SystemExit("Root and newer-folder mp files differ: inspect variants before adopting")
    return {
        "origin":"index.html explicitly labels this build a test multiplayer version; mp_ scripts are add-on JS, not the original Construct event runtime",
        "authoritative_singleplayer":["data.deobfuscated.js","c2runtime.js","mp_itemtable.js","assets/gfx"],
        "count":len(rows), "scripts":rows,
    }

if __name__=="__main__":
    manifest=scan()
    OUT.parent.mkdir(parents=True,exist_ok=True)
    OUT.write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+"\n",encoding="utf-8")
    print(f"PASS: {manifest['count']} distinct mp_ scripts; exact mirrored copies; index.html loading order; provenance protected")
