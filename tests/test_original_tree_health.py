"""Original Construct tree health, Spawn and hit source contract."""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
raw = (ROOT / "data.deobfuscated.js").read_text(encoding="utf-8-sig")
project = json.loads(re.sub(r"(?<![A-Za-z0-9_.])-\s+(?=\d)", "-", raw))["project"]
trees = {184: 5221960099677299, 207: 1639594650416124,
         209: 7109935635557623, 576: 2584406677664733,
         577: 2788700463962024, 581: 5413722554242966}
hit = {184: 7227858680569601, 207: 8754965976446553,
       209: 5386206472407495, 576: 9193333425135665,
       577: 4761103342332918, 581: 3531921662641626}
tutorial = next(layout for layout in project[5] if layout[0] == "Tutorial")
defaults = {obj: set() for obj in trees}
for layer in tutorial[6]:
    for instance in layer[14]:
        if instance[1] in trees:
            defaults[instance[1]].add(instance[3][0][0])
assert all(values == {3} for values in defaults.values()), defaults

def walk(events):
    for event in events:
        if isinstance(event, list) and event and event[0] == 0:
            yield event
            if len(event) > 7:
                yield from walk(event[7])

events = {event[4]: event for event in walk(next(
    sheet[1] for sheet in project[6] if sheet[0] == "Game_events"))}
for obj, uid in trees.items():
    actions = events[uid][6]
    assert actions[0][0:2] == [508, 73]
    assert actions[0][5][0] == [4, obj]
    assert not any(action[0] == obj and action[1] == 88
                   and action[5][0] == [10, 0] for action in actions)
    damage = events[hit[obj]][6][0]
    assert damage[:2] == [obj, 87]
    assert damage[5] == [[10, 0], [7, [0, 1]]]
print("Verified original six tree defaults, procedural creation and hit decrement.")
