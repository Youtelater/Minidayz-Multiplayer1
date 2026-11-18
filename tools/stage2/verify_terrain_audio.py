#!/usr/bin/env python3
"""Validate terrain/audio event wiring and exact original RomFS WAV filenames.

WAV audio comes from converted original game media already checked into
romfs/wav. This never downloads, renames or substitutes different sound effects.
"""
from pathlib import Path
import re
import struct

root=Path(__file__).resolve().parents[2]
sounds=root/"romfs/wav"
required={
    "hard_ground_1.wav", "hard_ground_2.wav",
    "road_run_1.wav","road_run_2.wav",
    "forest_run_1.wav","forest_run_2.wav",
    "sand_run_1.wav","sand_run_2.wav",
    "snow_run_1.wav","snow_run_2.wav",
    "swamp_step_1.wav","swamp_step_2.wav",
    "pick1.wav","pick2.wav",
    "ambient.wav","night_loop.wav","rain_loop.wav","blizzard.wav",
    "md_winter_amb.wav","md_winter_night_amb.wav",
    "crafting.wav","drop1.wav","drop2.wav","zippero.wav",
    "geardrop1.wav","geardrop2.wav",
}
# Construct source has choose(*_1,*_2,*_3,*_4) for these
# material families; hard_ground uses the two authored variants.
for family in ("run","forest_run","road_run","sand_run","snow_run","swamp_step"):
    required.update(f"{family}_{i}.wav" for i in range(1,5))
available={p.name for p in sounds.glob("*.wav")}
missing=required-available
assert not missing, f"Missing original RomFS WAV files: {sorted(missing)}"

for name in required:
    p=sounds/name
    with p.open("rb") as fp:
        head=fp.read(12)
        assert len(head)==12 and head[:4]==b"RIFF" and head[8:12]==b"WAVE",name
        assert p.stat().st_size>44,name

audio=(root/"source/stage2_audio.cpp").read_text(encoding="utf-8")
scene=(root/"source/main.cpp").read_text(encoding="utf-8")
surface=(root/"include/minidayz/terrain_surface.hpp").read_text(encoding="utf-8")
for stem in ("run","road_run","forest_run","sand_run","snow_run","swamp_step"):
    assert re.search(r'prefix = "'+stem+'"',audio),stem
for symbol in ("play_footstep","play_item_pickup",
               "set_environment_ambience","start_rain_loop"):
    assert symbol in scene,symbol
assert 'night_loop.wav' in audio and 'ambient.wav' in audio
assert 'md_winter_amb.wav' in audio and 'md_winter_night_amb.wav' in audio
assert 'wave.looping=(which==6 || which==12)' in audio
assert "FootstepSurface" in surface
assert "source_variant" in audio
assert "step_serial_++ % 4u" in audio
assert "kSourceFootstepSeconds=0.25f" in scene
assert "kSourceDefaultRainsLevel=2" in scene
assert "blizzard.wav" in audio
all_wavs=list(sounds.glob("*.wav"))
print(f"Verified {len(required)} original terrain/gameplay WAVs out of "
      f"{len(all_wavs)} supplied RomFS WAV files, audio event connections, "
      "and day/night loop channels")
