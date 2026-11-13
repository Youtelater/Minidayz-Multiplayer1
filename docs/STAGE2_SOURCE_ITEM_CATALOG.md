# Source-backed native item catalog

The build generates `include/minidayz/generated/item_source_catalog.hpp`
from the original English item XML, `MDZItemTable`, and parsed Construct
object table. It covers all 405 item IDs and preserves ID gaps.

Each row contains the original ID, localized name, and exact Construct object
index (65535 means no object mapping was found). It does not assign guessed
hunger, thirst, healing, protection, stack, or backpack capacity values.
Those effects remain tied to the original event trace until the ACE action
semantics and expressions have been ported into runtime code.
