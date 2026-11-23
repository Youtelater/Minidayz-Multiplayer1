#!/usr/bin/env python3
"""Guard the original Construct terrain coordinate/frame rendering contract.

Terrain must be drawn by original 60px/30px tile frame from assets/gfx,
not by stretching entire source PNG tilemap sheets. The native 3DS must
also avoid allocating source layers it never draws.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
engine = (root / "source/engine.cpp").read_text(encoding="utf-8")
scatter = (root / "source/terrain_scatter_render.cpp").read_text(encoding="utf-8")
main = (root / "source/main.cpp").read_text(encoding="utf-8")
ground = (root / "tools/stage1/build_ground_floor.py").read_text(encoding="utf-8")
details = (root / "tools/stage1/build_environment_tilemaps.py").read_text(encoding="utf-8")
assert 'original_map_ground_layout.bin' in engine
assert 'original_map_details_layout.bin' in engine
assert 'source_tile_transform(descriptor)' in engine
assert 'source_tile_page(descriptor)' in engine
assert 'source_tile_frame(descriptor)' in engine
assert 'draw_original_tile(floor_sheets_[page],descriptor' in engine
assert 'draw_original_tile(detail_sheets_[slot],frame' in engine
assert 'original_map_base_ground_lvl%u.t3x' in engine
assert 'ground_texture_at(' in engine
assert 'map.object_id!=terrain::kGroundObjectByTexture[map_theme]' in engine
assert 'if(map.layer>=44)' in engine
assert 'std::fseek(file,static_cast<long>(cell_count*sizeof(std::uint32_t))' in engine
assert 'if(map.layer>=44)continue;' in engine
assert 'draw_original_ground_floor(' in main
assert 'draw_original_environment_tilemaps(' in main
road=engine.split("bool Renderer::draw_source_road_grid(",1)[1].split(
    "bool Renderer::draw_source_sprite_fitted(",1)[0]
assert 'std::strcmp(source_layer.name,"asphalt")==0' in road
assert 'source_world_layer_depth(' in road
assert 'C2D_SpriteSetDepth(&sprite,road_depth)' in road
assert 'world_source::mapgen_road_band(' in road
assert 'for(int row=0;row<grid.height();++row)' in road
assert 'bounds.width/grid.width()' not in road
assert 'draw_source_procedural_environment(' in main
# Each instance may be in a different original Construct animation (e.g.
# closed/open doors and damaged/standing trees). Do not regress to always
# drawing animation 0 just to satisfy the old static-render contract.
assert 'renderer.release_menu_sheets();' in main
assert 'void Renderer::release_menu_sheets() noexcept' in engine
assert 'draw_source_sprite(placement->object,' in main
assert 'placement->source_animation, placement->source_frame' in main
assert 'sprite->animation==placement->source_animation' in main
assert 'px, py, sx, sy, layer_depth' in main
assert 'source_sprite_in_view(' in main
assert 'original_map_detail_230.t3x' in scatter
assert 'kTerrainScatterChoices' in (root/'source/world.cpp').read_text(encoding='utf-8')
assert 'world_source::mapgen_hotspot(' in scatter
assert 'bounds.width/grid.width()' not in scatter
assert 'world_source::mapgen_hotspot(x,y)' in (
    root/"source/world.cpp").read_text(encoding="utf-8")
assert 'source_world_layer_depth(' in scatter
assert 'stage1::kSourceScatterGroundLayer' in scatter
assert 'C2D_SpriteSetDepth(&sprite,0.17f)' not in scatter
scatter_generator=(root / "tools/stage1/build_procedural_environment.py").read_text(encoding="utf-8")
assert 'source_scatter_layer=ground_layers.pop()' in scatter_generator
scatter_header=(root / "include/minidayz/generated/terrain_scatter_source.hpp").read_text(encoding="utf-8")
assert 'kSourceScatterGroundLayer=6;' in scatter_header  # Map layer 6 is ground_enviroment
assert 'cells.fill(kEmpty)' in (root/'source/world.cpp').read_text(encoding='utf-8')
assert 'world.source_scatter_patches()' in scatter
assert 'std::minstd_rand' not in scatter
assert 'original_map_ground_p{page:03}.t3x' in ground
assert 'source.crop((x, y, x + tile_width, y + tile_width))' in ground
assert "source_base_tile != 6" in ground
assert "base_id = source_base_tile" in ground
assert "selected = original_base_id" in (root/"tools/stage1/verify_ground_floor.py").read_text(encoding="utf-8")
assert "std::floor(std::clamp(" in main

assert 'art.crop((x,y,x+tile_size,y+tile_size))' in details
assert 'original_map_detail_{object_id}.t3x' in details
print('PASS: terrain coordinates, original frames, seven level themes, '
      'visible tilemap storage, and runtime scatter integration')
