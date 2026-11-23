#!/usr/bin/env python3
"""No original Construct event can silently vanish or be marked implemented."""
from pathlib import Path
from collections import Counter
import json
ROOT=Path(__file__).resolve().parents[1]
data=ROOT/"audit-output/source-parity"
summary=json.loads((data/"parity_summary.json").read_text())
assert summary["counts"]["events"]==22343
assert summary["counts"]["seven_field_leaf_events"]==14560
assert summary["counts"].get("reviewed_event_mappings",0)==len(json.loads(
  (ROOT/"data/parity/verified_original_events.json").read_text())["verified"])
assert summary["counts"]["conditions"]>0 and summary["counts"]["actions"]>0
records=[json.loads(line) for line in
  (data/"every_original_event.jsonl").read_text().splitlines()]
assert len(records)==22343
assert len({r["key"] for r in records})==len(records)
assert all("inherited_conditions" in r and
  "conditions" in r and "actions" in r and
  "parent_uids" in r and "fingerprint" in r for r in records)
assert all(r["verification"]=="not_event_verified" or
  (r["implementation"] and r["regression_test"]) for r in records)
# Every one of the root Construct's 63,289 actions must be present,
# identified by exact event path+ordinal, without claiming 3DS behavior.
import gzip
seen=set()
with gzip.open(data/"every_original_action.jsonl.gz","rt",encoding="utf-8") as ledger:
 for line in ledger:
  entry=json.loads(line)
  assert entry["action_key"] not in seen
  seen.add(entry["action_key"])
  assert "original_action" in entry and "own_conditions" in entry
  assert "inherited_conditions" in entry and "parent_event_uids" in entry
  assert entry["native_action_parity"]=="not_individually_verified"
assert len(seen)==summary["action_occurrences_indexed"]==63289

cars=json.loads((data/"original_car_loot_event_manifest.json").read_text())
assert cars["source_event_occurrences"]>=8
assert all(not v["native_car_interaction_implemented"] for v in cars["branches"])
assert all(v["item_expression_kind"] in (
  "original_choose","original_literal","original_dynamic_expression")
  and v["source_action"][5][0]==[1,[2,"Spawn_drop"]] for v in cars["branches"])
assert all(v["item_choices_weighted"] for v in cars["branches"]
 if v["item_expression_kind"] != "original_dynamic_expression")
assert 226004896143721 in [v["event_uid"] for v in cars["branches"]]
print("PASS: 22,343 events and 63,289 individual actions indexed; no silent omissions or false car implementation")
