# Stage 2 original-source audit

## Authoritative source boundary

The authoritative game version is the original Mini DAYZ 1.6.0 source stored at the repository root. The `Minidayz-Multiplayer-main/` directory is explicitly excluded from runtime, catalog, extraction, and asset inputs.

- Construct export: root-level `data.deobfuscated.js` (14,558,329 bytes).
- Item object and sprite identities: root-level `mp_itemtable.js`.
- Item names: root-level `l_eng_items.xml` (405 item records).
- Item descriptions: root-level `l_eng_new.xml` (528 description records).
- UI strings and labels: root-level `l_eng_ui.xml`.
- Player feedback/log strings: root-level `l_eng_log.xml`.
- Original artwork: root-level `assets/gfx/`, `images/`, and `gfx/`.

## Item-description coverage

The 20 item IDs without a matching entry in `l_eng_new.xml` are genuine source-data gaps, not a failed build or a reason to borrow newer-version text:

`999, 230, 231, 239, 242, 245, 250, 300, 350, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 400`.

Keep these descriptions absent unless another root-level original source provides an exact description. The 143 extra description records are preserved as source data; they are not silently assigned to unrelated item IDs.

## Extraction and build audit

The Stage 1 extraction pipeline rebuilds its snapshot byte-for-byte from root-level `data.deobfuscated.js`, parses the Construct project, validates generated inventories against the parsed original, and resolves original frame references against root-level artwork. The Stage 2 item, sprite, inventory behavior, direct-use effect, and equipment catalogs are generated from root-level source inputs.

The item-name catalog helper was corrected to read root-level `l_eng_items.xml`; it must never read from the excluded newer-version directory. The stack-size behavior documentation likewise references the root-level Construct export.

## Original gameplay/UI evidence captured

- The root Construct export includes the `Check_stack_size` function, the `perk_cybb` branch, the `Backpack` equipment slot, and the `player_backpacks` visual layer.
- The original player layers include `player_pants`, `player_hands`, `player_clothing`, `player_vests`, `player_helmets`, `player_backpacks`, and `player_weapons`.
- Raw original inventory event conditions/actions are exported with source UIDs; direct item-use stat effects and equipment properties are separately extracted only where source expressions are unambiguous.
- Original UI text is available in `l_eng_ui.xml`; the source UI artwork is available in the root artwork directories. Full UI implementation remains a later Stage 2 workstream, not a prerequisite to completing this source-inventory audit.

## Verification boundary

Native regression and 3DS CI builds passed on the previously verified commit. After the source-path correction in this audit, both workflows must be rerun on the same new commit before claiming fresh CI verification. CI build success is not a physical 3DS hardware test.

## Step 1 result

**Original-source inventory and provenance audit complete.** The remaining 20 missing descriptions are explicitly documented as absent from the authoritative original files rather than fabricated. Functional work that consumes these verified sources (backpack capacity, remaining equipment/item actions, and full UI) remains in its ordered Stage 2 implementation steps.
