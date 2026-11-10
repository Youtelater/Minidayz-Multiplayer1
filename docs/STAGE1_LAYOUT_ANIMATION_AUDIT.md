# Stage 1 — Layout, layer, and animation audit

Source: authoritative `data.deobfuscated.js` export, extracted inventories under `data/stage1/`, and the repository's original `assets/gfx/` files. Branch: `main`.

This audit records source structure for the native renderer. It is not a claim that all Construct event behavior or animation playback has already been ported.

## 1. Layout inventory

| Layout | Source size | Placed instances | Event sheet | Purpose indicated by source structure |
|---|---:|---:|---|---|
| `GUI_layout` | 1024 × 768 | 302 | `Game_events` | Main game/HUD composition |
| `Polygon` | 1500 × 1500 | 1,606 | `Game_events` | Polygon/terrain or collision reference layout |
| `Map` | 36,850 × 20,000 | 1,250 | `Game_events` | Main world |
| `TD` | 36,850 × 20,000 | 978 | `Game_events` | Alternate world/game mode layout |
| `Tutorial` | 8,300 × 2,000 | 754 | `Game_events` | Tutorial world |
| `Menu` | 2,500 × 1,500 | 330 | `Menu_Events` | Main menu |
| `Login` | 850 × 890 | 30 | `Login_events` | Login flow |
| `Loading` | 2,000 × 1,500 | 6 | `Loading_events` | Loading flow |

Total: 8 layouts, 360 layers, 5,256 placed instances. These source dimensions are not the Nintendo 3DS screen dimensions; the native renderer must preserve the original layout coordinate system and apply a deliberate camera/viewport transform for the 3DS.

## 2. Main world layer order

The `Map`, `TD`, and `Tutorial` layouts share the same 69-layer structure. The following order is taken from the extracted `Map` layer table (zero-based layer indices):

| Index | Layer | Rendering/system role indicated by its name |
|---:|---|---|
| 0–8 | `Layer 0`, `bg`, `snow`, `sand`, `swamp`, `asphalt`, `ground_enviroment`, `rain_drops`, `Obstacle_enviroment` | Base, terrain, weather effects, obstacles |
| 9 | `Buildings_shadows` | Building shadows |
| 10–16 | `Survivors2`, `s_pants2`, `s_clothing2`, `s_vests2`, `s_helmets2`, `s_backpack2`, `s_weapons2` | Secondary survivor and equipment layers |
| 17–20 | `Buildings_base`, `items_on_ground`, `Water`, `Buildings_base_top` | Structure bases, ground loot, water, structure overlays |
| 21–28 | `Bullets`, `Survivors`, `S_pants`, `S_clothing`, `s_vests`, `s_helmets`, `s_backpack`, `s_weapons` | Projectiles, survivors, clothing and equipment |
| 29–40 | `Zeds`, `vehicles`, `Buildings_bg`, `player`, `player_pants`, `player_hands`, `player_clothing`, `player_vests`, `player_helmets`, `player_backpacks`, `player_weapons`, `static_cars` | Zombies, vehicles, building backgrounds, player and equipment |
| 41–48 | `Buildings`, `Forests`, `Trees`, `Rain`, `nvg`, `night`, `Color_effect`, `Sunset_Dawn` | Foreground structures, vegetation, weather and lighting |
| 49–52 | `zed_resp`, `LOOT_respawn`, `Bot_respawn`, `Tourist_points` | Spawn/control points; `zed_resp` and `LOOT_respawn` are hidden in the Map layout |
| 53–60 | `Debug_panel`, `GUI_controls`, `GUI`, `Rust_layer`, `GUI_overs`, `GUI_drag`, `Gui_counters`, `GUI_elements` | Debug, controls, HUD, overlays, drag UI and counters |
| 61–68 | `Markers`, `TIPsLayer`, `Deadscreen`, `tutortext`, `GUI_hint`, `test`, `fade`, `metric` | Markers, tutorial hints, death screen and final overlays |

The layer names and ordering are source facts. The role descriptions are conservative interpretations of those names; exact draw behavior, visibility changes, and z-order interactions still need to be traced through the event sheets.

## 3. Other layout layer structures

- `GUI_layout`: 63 layers. Includes player/equipment, Zeds, debug, GUI controls, GUI, counters, UI elements, death screen, tips and fade layers.
- `Polygon`: 64 layers. Shares the world terrain, structure, survivor, zombie, player, weather, and UI stack; its largest placed-object layer is `Buildings` (905 instances).
- `Menu`: 17 layers. Includes background/ground, building shadows, Zeds, buildings, buttons, menu elements, update elements, achievements, and stick controls.
- `Login`: 4 layers: `Layer 0`, `bg`, `light`, `elements`.
- `Loading`: 5 layers: `test`, `sky`, `menu_elements`, `city`, `loader_bar`.

## 4. Animation inventory

Extracted from `data/stage1/animations.csv`:

- 6,338 animation definitions across 1,222 object indices.
- 14,505 total image-frame references across those definitions.
- Animation frame counts range from 1 to 789.
- 1,657 animations are marked looping; 4,681 are marked non-looping.
- Recorded animation speed values are 0 or 1.

The CSV preserves the source object index, animation name, frame count, speed, and loop flag. The current generated object metadata does not yet provide a verified human-readable name mapping for every numeric object index. Therefore this audit deliberately does not invent object names or convert the source speed field into milliseconds. Exact frame rectangles, per-frame durations, origins, imagepoints, collision polygons, and event-driven animation transitions still need to be resolved against the Construct object/animation data and original artwork.

## 5. Sprite metadata cross-check

`gfx/spritesheet.meta.json` reports metadata version 2, a single visible `Base` layer, four columns, and manual cell settings of 512 × 512. It lists 290 sprite entries. Individual entries carry source image names and dimensions. This is useful as an artwork inventory, but it is not sufficient by itself to reconstruct Construct animation frame rectangles or runtime timing. Original media remains unchanged.

## 6. Native implementation implications

1. Keep source layout coordinates separate from the 3DS screen resolution; use a camera/viewport transform rather than changing source positions.
2. Preserve the original layer order and visibility per layout. The world, menu, login, loading, and HUD should not be flattened into one draw list without layer semantics.
3. Render player clothing/equipment as separate source layers where the original uses them; do not replace the layered player with a single guessed sprite.
4. Keep animation metadata in source units until the exact Construct timing semantics are resolved.
5. Tie frame selection and transitions to the event-sheet behavior and exact source image references before calling the sprite animation port equivalent.

## 7. Open work

- Resolve each animation's object index to the exact Construct object/type and sprite resource.
- Extract per-frame source rectangles, origins, imagepoints, collision polygons, and animation timing.
- Trace event-driven layer visibility, opacity, parallax, effects, and animation changes.
- Build a source-to-native render mapping and compare screenshots/layout states against the original.
- Resolve the Construct ACE command metadata blocker documented in `docs/STAGE1_COMMAND_ID_MAPPING.md` before translating dependent event actions/conditions.

**Status: layout and layer structure inventoried; animation counts/flags inventoried; exact sprite-to-object mapping, runtime timing, and behavior-equivalent native rendering remain open.**
