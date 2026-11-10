# Stage 1 — Original sprite catalog and renderer

## Source of truth

The sprite catalog is generated from the original Construct project snapshot at
`data/stage1/construct_project_parsed.json`, reconstructed from
`data/stage1/source_snapshot/part_*.jsonpart`. It does not infer sprite
identity from filenames or substitute artwork from `romfs/`.

For each original object and animation frame, the generator preserves:
- original object index, animation index, and frame index;
- original image-sheet filename;
- exact source crop rectangle (x, y, width, height);
- exact frame origin (origin_x, origin_y);
- stable image index in the generated Tex3DS atlas.

Identical source rectangles in the same source sheet are deduplicated. Atlas
pages are bounded to 128 subtextures to avoid creating oversized single atlases.
Original files in `assets/gfx/` are read-only; all crops are temporary build
files and generated `.t3x` atlases are written under
`romfs/gfx/sprites/`.

## Build pipeline

The Makefile now performs these steps before compiling:
1. Reassemble the source snapshot.
2. Regenerate the structural inventories from the parsed original project.
3. Run `tools/stage1/build_sprite_catalog.py` to crop exact source frames,
   build Tex3DS atlases, and generate
   `include/minidayz/generated/sprite_catalog.hpp`.
4. Run the existing original-PNG texture conversion pipeline.

## Runtime API

`Renderer::draw_source_sprite(object, animation, frame, x, y, scale_x, scale_y, depth)`
resolves the exact source frame metadata, loads the corresponding atlas page
into a 32-entry round-robin texture cache, applies the source origin, and draws
the atlas subtexture by its recorded index.

The cache is bounded because the original project has many source sheets and
3DS texture memory is limited. A sheet may be reloaded after eviction.

## Remaining work — do not mark renderer complete yet

- Map placed world instances and live runtime objects to the exact object,
  animation, and frame IDs at their source coordinates and layers.
- Add animation playback using the source animation frame order, speed, loop
  flags, and event-driven animation changes.
- Add Construct layer ordering, visibility, opacity, parallax, and camera rules.
- Verify sprite atlas output and memory/performance on Manic Emu and hardware.
- Compare rendered screenshots against the original game.
- Connect the player, buildings, terrain, items, zombies, and HUD to this API.

**Status:** exact source-frame catalog generation and a native source-frame draw
API have been added. Full scene/object mapping and visual equivalence are not
yet complete.
