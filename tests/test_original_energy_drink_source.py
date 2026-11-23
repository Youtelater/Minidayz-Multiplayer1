#!/usr/bin/env python3
"""Root-original Construct event guard for the Edrink movement modifier.

Checks full source ancestry instead of copying guessed item effects. Indexes
seven-field leaf events as well as eight-field parents. Source extraction
alone does NOT certify native runtime execution, so pair with C++ tests.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/stage1"))
from audit_original_world_exhaustive import decode

project = decode((ROOT / "data.deobfuscated.js").read_bytes())
sheets = dict(project[6])
assert "Game_events" in sheets, "Root Game_events missing"


def visit(nodes, parents=(), path=()):
    for ordinal, event in enumerate(nodes if isinstance(nodes, list) else ()):
        if not isinstance(event, list) or len(event) < 7:
            continue
        if event[0] != 0:
            if len(event) > 7:
                yield from visit(event[7], parents, path + (ordinal,))
            continue
        current = path + (ordinal,)
        yield current, event, parents
        if len(event) > 7:
            yield from visit(event[7], parents + (event,), current)


def numeric_param(call, kind):
    # Construct condition records carry parameters in slot 9, whereas
    # action records carry them in slot 5. The intervening condition flags
    # include booleans and must never be interpreted as the param list.
    params = (call[9] if len(call) > 9 else call[5]) if (
        isinstance(call, list) and len(call) > 5) else []
    for row in params:
        if not isinstance(row, list) or row[:1] != [kind]:
            continue
        if kind == 10 and len(row) > 1 and isinstance(row[1], int):
            return row[1]
        if kind == 7 and len(row) > 1 and isinstance(row[1], list) and (
            len(row[1]) == 2 and row[1][0] == 0
        ):
            return row[1][1]
    return None


def called(event, obj, command):
    return [a for a in event[6] if isinstance(a, list) and len(a) > 5
            and a[0] == obj and a[1] == command]


def text_present(value, name):
    if isinstance(value, str):
        return value == name
    return isinstance(value, list) and any(text_present(v, name) for v in value)


records = list(visit(sheets["Game_events"]))
use = [(path, e, ancestors) for path, e, ancestors in records
       if e[4] == 435941792839578]
expiry = [(path, e, ancestors) for path, e, ancestors in records
          if e[4] == 197231340522492]
assert len(use) == 1 and len(expiry) == 1, "Source Edrink event identity changed"

path, event, ancestors = use[0]
assert any(
    c[0:2] == [189, 112] and numeric_param(c, 7) == 63
    for parent in ancestors for c in parent[5]
    if isinstance(c, list) and len(c) > 5
), "Original item63 activation guard missing"

# Original audio action is not optional: item use plays the canonical
# media/drinksoda_0 recording at -10 dB before starting the source timer.
assert any(a[0:2] == [187, 111] and text_present(a[5], "DrinkSoda_0")
           and [0,[0,-10]] in a[5]
           for a in event[6] if isinstance(a,list) and len(a)>5), (
    "Missing original DrinkSoda_0(-10dB) audio action")
assert (ROOT / "romfs/wav/drinksoda_0.wav").is_file()

timer_start = [a for a in called(event, 181, 106)
               if text_present(a[5], "Edrink")]
assert len(timer_start) == 1, "Missing original Timer.Start Edrink action"
assert numeric_param(timer_start[0], 0) is None  # not a set-variable command
assert text_present(timer_start[0][5], "Edrink")
assert any(x == [0, [0, 60]] for x in timer_start[0][5])
assert any(numeric_param(a, 10) == 4 and numeric_param(a, 7) == 15
           for a in called(event, 193, 88)), "PlayerBase var4 +15 missing"

_, stop_event, _ = expiry[0]
assert any(text_present(c[9], "Edrink") for c in stop_event[5]
           if isinstance(c, list) and len(c) > 9 and c[0:2] == [181, 107])
assert any(numeric_param(a, 10) == 4 and numeric_param(a, 7) == 0
           for a in called(stop_event, 193, 88)), "Timer expiry var4 reset missing"
print(f"PASS: source item63 at Game_events:{'.'.join(map(str,path))} "
      "starts Edrink(60s), PlayerBase193.var4=15; expiry resets var4=0")
