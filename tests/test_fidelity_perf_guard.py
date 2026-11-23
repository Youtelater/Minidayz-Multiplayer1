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
spawn=main.split("bool place_new_player_on_generated_map(",1)[1].split(
    "void begin_world_generation() noexcept {",1)[0]
assert "destination.source_random()" in spawn and "destination.generation_level()" in spawn
assert "destination.seed() ^" not in spawn
assert "static_cast<double>(source_rng())/4294967296.0" in spawn
assert "std::uniform_real_distribution" not in spawn
assert "kNormalPlayerStartX" in spawn and "kNormalPlayerStartYBase" in spawn
assert "kSnowPlayerStartXMin" in spawn and "kSnowPlayerStartY" in spawn
assert "blocked(this," not in spawn and "local_offsets" not in spawn
assert "for(int attempt" not in spawn and "state.x=px;" in spawn
assert "delta_seconds_=std::clamp(dt,0.0f,0.25f);" in engine
# Source roads must render from their own atlas even when the original
# ground RLE layout is damaged. Otherwise both terrain and roads vanish.
road_renderer=engine.split("bool Renderer::draw_source_road_grid(",1)[1].split(
    "bool Renderer::draw_source_sprite_fitted(",1)[0]
assert "ground_loaded_" not in road_renderer
assert "ensure_road_sheet(theme)" in road_renderer
assert 'std::strcmp(source_layer.name,"asphalt")==0' in road_renderer
assert "source_world_layer_depth(" in road_renderer
assert "C2D_SpriteSetDepth(&sprite,road_depth)" in road_renderer
assert "C2D_SpriteSetDepth(&sprite,0.10f)" not in road_renderer
assert "clamp(delta_seconds, 0.0f, 0.25f)" in player
# The renderer tries the real selected survivor first; only a missing
# source sprite may fall back to the original player_skin_def_2 object.
# Never silently replace healthy selected skins with the default.
assert "source_skin_object!=minidayz::kSourcePlayerObject" in main
assert "if(p.alive && !sprite_drawn &&" in main
assert "minidayz::kSourcePlayerObject,skin_animation," in main
assert main.index("source_skin_object,\n                skin_animation") < main.index(
    "minidayz::kSourcePlayerObject,skin_animation,")
assert "std::min(value, 0.25f)" in npc
assert "sprite_cache_misses_" in engine
assert "KEY_SELECT" in engine and "measured_frame_seconds_" in engine
assert "scatter_cache_" not in header
assert "world.source_scatter_patches()" in terrain
assert "std::minstd_rand" not in terrain
assert "source_scatter_patches_" in (ROOT/"include/minidayz/world.hpp").read_text()
# The original Tilemap230 ground_enviroment layer is source layer 6, not
# a guessed GPU constant. Test the source-derived depth path.
assert "source_world_layer_depth(" in terrain
assert "stage1::kSourceScatterGroundLayer" in terrain
print("PASS: original-spawn relocation removed, slow frames retained, "
      "cached source terrain and emulator profiling wired")
