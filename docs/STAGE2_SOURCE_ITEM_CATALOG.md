# Source-backed native item catalog

The build generates `include/minidayz/generated/item_source_catalog.hpp`
from the original English item XML, `MDZItemTable`, and parsed Construct
object table. It covers all 405 item IDs and preserves ID gaps.

Each row contains the original ID, localized name, and exact Construct object
index (65535 means no object mapping was found). It does not assign guessed
hunger, thirst, healing, protection, stack, or backpack capacity values.
Those effects remain tied to the original event trace until the ACE action
semantics and expressions have been ported into runtime code.


## Current extraction result

The native build regenerates all three catalogs from the checked-in original source snapshot:

- 405 localized item IDs/names.
- 405 source item rows, with 329 resolved Construct object identities and 76 explicitly unresolved IDs.
- 329 exact item-ID-to-Construct-sprite object mappings from the original `MDZItemTable`.
- 2,223 raw inventory behavior event records exported by `extract_item_behavior_trace.py`.

Unresolved mappings are deliberately left without a sprite rather than borrowing an unrelated icon. The native renderer resolves icons through the generated source sprite catalog, and regression tests verify that every mapped sprite points back to the same original Construct object.
