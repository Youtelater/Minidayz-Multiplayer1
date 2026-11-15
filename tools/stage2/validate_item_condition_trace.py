#!/usr/bin/env python3
"""Validate that original item-condition write events remain in the Stage 2 trace.

This is deliberately a provenance check, not a gameplay interpretation: Construct
command 87/88 parameters are retained verbatim until their object semantics and
perk expressions are translated into the native runtime.
"""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TRACE = ROOT / "data/stage2/item_behavior_trace.json"

GROUPS = {
    6904772320561696: ("Firearm shooting", 875054325568454),
    337954396197555: ("Alternate attacks", 875054325568454),
    3528035038609865: ("Melee fight", 2098275646157213),
    811368909310732: ("Damage dealing", 251602875520018),
    9520121190252054: ("Player damaging", 682686209618980),
    5836740205504893: ("Hitting_obstacles", 8036449108462062),
}


def is_condition_write(action: object) -> bool:
    if not isinstance(action, list) or len(action) < 6:
        return False
    if action[0] != 1030 or action[1] not in (87, 88):
        return False
    params = action[5]
    return (
        isinstance(params, list)
        and bool(params)
        and isinstance(params[0], list)
        and len(params[0]) >= 2
        and params[0][0] == 10
        and params[0][1] == 6
    )


def main() -> None:
    if not TRACE.is_file():
        raise SystemExit(f"Missing generated original item behavior trace: {TRACE}")
    trace = json.loads(TRACE.read_text(encoding="utf-8"))
    if trace.get("item_count") != 405:
        raise SystemExit("Condition trace must retain all 405 original item IDs")

    by_uid = {group.get("source_uid"): group for group in trace.get("groups", [])}
    missing = set(GROUPS) - set(by_uid)
    if missing:
        raise SystemExit(f"Missing original condition-wear event groups: {sorted(missing)}")

    total = 0
    for source_uid, (name, required_event_uid) in GROUPS.items():
        group = by_uid[source_uid]
        events = group.get("events", [])
        if group.get("source_name") != name:
            raise SystemExit(
                f"Source group {source_uid} renamed unexpectedly: "
                f"{group.get('source_name')!r}"
            )
        if group.get("event_count") != len(events):
            raise SystemExit(f"Event count mismatch in original group {name}")

        writes = [
            event for event in events
            if any(is_condition_write(action) for action in event.get("actions", []))
        ]
        if not any(event.get("uid") == required_event_uid for event in writes):
            raise SystemExit(
                f"Original condition write {required_event_uid} missing from {name}"
            )
        total += len(writes)
        print(f"{name}: {len(writes)} original item-condition write events traced")

    print(f"Validated {total} source-traced item-condition write records across six groups")


if __name__ == "__main__":
    main()
