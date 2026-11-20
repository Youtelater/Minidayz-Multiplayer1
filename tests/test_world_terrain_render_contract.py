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
assert 'draw_source_procedural_environment(' in main
assert 'draw_source_sprite(placement->object, 0, placement->source_frame' in main
assert 'px, py, sx, sy, layer_depth' in main
assert 'source_sprite_in_view(' in main
assert 'original_map_detail_230.t3x' in scatter
assert 'kTerrainScatterChoices' in scatter
assert 'patch->cells.fill(kEmpty)' in scatter
assert 'original_map_ground_p{page:03}.t3x' in ground
assert 'source.crop((x, y, x + tile_width, y + tile_width))' in ground
assert 'art.crop((x,y,x+tile_size,y+tile_size))' in details
assert 'original_map_detail_{object_id}.t3x' in details
print('PASS: terrain coordinates, original frames, seven level themes, '
      'visible tilemap storage, and runtime scatter integration')
