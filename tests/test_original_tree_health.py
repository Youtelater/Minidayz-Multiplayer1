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

# Original projectile guard ancestry and ordered sound/drop expressions.
collision = events[5202978245239527]
assert collision[5][0][:2] == [1031, 71]
assert collision[5][0][9] == [[10, 4], [8, 1], [7, [2, "fire"]]]
assert collision[5][1][:2] == [1031, 178]
assert collision[5][1][9] == [[4, 345]]
flag = events[8407650361789881]
assert flag[5][0][:2] == [1031, 91]
assert flag[5][0][9] == [[10, 7]]
fall_counts = {184: 2, 207: 3, 209: 4, 576: 2, 577: 3, 581: 4}
for obj, uid in hit.items():
    event = events[uid]
    assert event[5][0][9] == [[8, 0], [0, [0, 10]]]
    assert event[5][1][9] == [[4, obj]]
    assert event[5][2][9] == [[10, 0], [8, 4], [7, [0, 0]]]
    assert event[6][1][5][1] == [1, [19, 82, [[2, "chop"], [2, "chop2"], [2, "chop3"], [2, "chop4"]]]]
    fall = event[7][0]
    assert fall[6][0][5][1] == [1, [19, 82, [[2, "treefall_02"], [2, "treefall_03"]]]]
    drops = [a for a in fall[6] if a[:2] == [189, 69]]
    assert len(drops) == fall_counts[obj]
    for action in drops:
        args = action[5][1]
        assert args[2] == [7, [4, [20, obj, 65, False, None], [19, 95, [[0, -30], [0, 30]]]]]
        assert args[3] == [7, [4, [4, [20, obj, 66, False, None], [0, 40]], [19, 95, [[0, 10]]]]]
    assert fall[6][-1][5] == [[11, "tutorTreeChopped"], [7, [0, 1]]]
print("Verified source projectile guards, sound choices, random coordinates and tutorial flag.")
