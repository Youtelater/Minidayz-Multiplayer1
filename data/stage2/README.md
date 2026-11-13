# Stage 2: Items and inventory

Stage 2 begins with a source-backed item catalog and native inventory rules.

- Minidayz-Multiplayer-main/l_eng_items.xml is the authoritative English item-name source.
- tools/stage2/extract_item_catalog.py generates include/minidayz/generated/item_catalog.hpp.
- Item IDs are preserved exactly, including gaps. ID 0 is the empty slot and cannot be added.
- InventorySystem provides fixed 15-slot storage, atomic additions, caller-supplied stack limits, removal, stack transfer, and complete-stack swapping.
- Stack limits and item effects are deliberately not guessed. They must be ported from original gameplay data/events before being configured.
- Loot spawning, pickup interaction, equipment effects, and item use are the next Stage 2 integration tasks.
