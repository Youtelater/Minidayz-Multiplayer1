# Stage 2 original-source audit

- Authoritative original source: `data.deobfuscated.js` (Library file, 14,558,316 bytes).
- SHA-256: `b5e15cfad3106b56457bc13c4169e59b4ff0308035912330bf144b86238571d3`.
- Original project title/version: Mini DAYZ 1.6.0.
- The original source contains the `Check_stack_size` event and `perk_cybb` branch.
- Original item IDs and Construct object names are mapped by `MDZItemTable`; localized names are in `l_eng_items.xml`.
- The source-backed native item catalog currently preserves 405 item IDs and gaps.
- The original source identifies a dedicated `Backpack` equipment slot and the `player_backpacks` visual layer.

Do not infer item effects or backpack slot counts from item names. The raw source event expressions remain the authority for those values; add native constants only after extracting the matching source expression and event UID.


## Verified build outputs

The native CI run generated 405 original item definitions, resolved 329 item-to-Construct sprite mappings, and extracted 2,223 original inventory behavior event records. Both the native regression workflow and the 3DS executable build completed successfully after the inventory fixes. The 76 item IDs without an `MDZItemTable` mapping remain explicitly unresolved; no substitute art or item effects are fabricated.
