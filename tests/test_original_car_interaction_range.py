#!/usr/bin/env python3
"""Original Construct mobile car distance comparator vs native A adaptation."""
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools/stage1"))
from audit_original_world_exhaustive import decode
project=decode((ROOT/"data.deobfuscated.js").read_bytes())
def walk(nodes):
    for e in nodes if isinstance(nodes,list) else ():
        if isinstance(e,list) and len(e)>=7 and e[0]==0:
            yield e
            if len(e)>7:yield from walk(e[7])
events=[e for sheet,src in project[6] if sheet=="Game_events"
        for e in walk(src) if e[4]==711142196261088]
assert len(events)==1, "Original car mobile proximity event changed"
assert any(c[:2]==[-1,51] and len(c)>9 and c[9][1]==[8,3] and
           c[9][2]==[7,[0,100]] and isinstance(c[9][0],list) and c[9][0][0]==7 and
           isinstance(c[9][0][1],list) and c[9][0][1][:2]==[19,79]
           for c in events[0][5])
main=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
generator=(ROOT/"tools/stage2/build_original_car_trunk_loot.py").read_text(encoding="utf-8")
assert "distance=minidayz::items::kOriginalCarTrunkInteractionRange" in main
assert "source distance < 50" not in main
assert "kOriginalCarTrunkInteractionRange=100.0f" in generator
print("PASS root mobile car distance<=100 uses original comparison/extent in 3DS")

audio=(ROOT/"source/stage2_audio.cpp").read_text(encoding="utf-8")
scene=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
enum=(ROOT/"include/minidayz/stage2_audio.hpp").read_text(encoding="utf-8")
assert '"romfs:/wav/open.wav"' in audio
assert 'carTrunkOpen' in enum
assert 'OriginalWorldSound::carTrunkOpen' in scene
# One original Sprite241 search event must play one source cue, not two.
# Checking the interaction function catches duplicated playback without
# mistaking unrelated scene-wide world sound calls for this car event.
search_start=scene.index("bool request_source_car_trunk() noexcept {")
search_end=scene.index("\n    }",search_start)
assert scene[search_start:search_end].count(
    "OriginalWorldSound::carTrunkOpen")==1, (
    "Original car trunk open sound is missing or played more than once")
assert (ROOT/"romfs/wav/open.wav").is_file()

assert audio.count("source_car_open")>=2
assert audio.count("0.56234133f")>=2
