#!/usr/bin/env python3
"""Fail regressions in source spawn fidelity and slow-frame movement.

The authoritative Start-events expressions are verified by
tools/stage1/build_player_spawn_source.py against data.deobfuscated.js.
This check rejects reintroduction of native relocation and movement time
loss, not a false claim of browser RNG equivalence.
"""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
main=(ROOT/"source/main.cpp").read_text(encoding="utf-8")
engine=(ROOT/"source/engine.cpp").read_text(encoding="utf-8")
player=(ROOT/"source/player.cpp").read_text(encoding="utf-8")
npc=(ROOT/"source/npc_system.cpp").read_text(encoding="utf-8")
terrain=(ROOT/"source/terrain_scatter_render.cpp").read_text(encoding="utf-8")
header=(ROOT/"include/minidayz/engine.hpp").read_text(encoding="utf-8")
spawn=main.split("bool place_new_player_on_generated_map() noexcept {",1)[1].split(
    "void begin_world_generation() noexcept {",1)[0]
assert "kNormalPlayerStartX" in spawn and "kNormalPlayerStartYBase" in spawn
assert "kSnowPlayerStartXMin" in spawn and "kSnowPlayerStartY" in spawn
assert "blocked(this," not in spawn and "local_offsets" not in spawn
assert "for(int attempt" not in spawn and "state.x=px;" in spawn
assert "delta_seconds_=std::clamp(dt,0.0f,0.25f);" in engine
assert "clamp(delta_seconds, 0.0f, 0.25f)" in player
assert "std::min(value, 0.25f)" in npc
assert "sprite_cache_misses_" in engine
assert "KEY_SELECT" in engine and "measured_frame_seconds_" in engine
assert "scatter_cache_" in header and "std::vector<ScatterCacheEntry>" in header
assert "patch->cells" in terrain and "if(!patch)" in terrain
assert "C2D_SpriteSetDepth(&sprite,0.17f)" in terrain
print("PASS: original-spawn relocation removed, slow frames retained, "
      "cached source terrain and emulator profiling wired")
