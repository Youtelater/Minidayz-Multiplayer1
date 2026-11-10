# Stage 1 native data access layer

This change adds a C++17 read-only access layer over the source-generated Construct
project tables. It gives gameplay modules a stable way to resolve layouts, layers,
objects, audio resources, placed instances, and per-object animation ranges without
copying or mutating original media.

## API

- `find_layout`, `find_object`, `find_layer`, and `find_audio` perform exact lookups.
- `instances_for_layout` and `instances_for_layer` return bounded table ranges.
- `animations_for_object` returns the contiguous animation range for one source object.
- `validate_data` checks generated table indices and range bounds before runtime use.

All APIs are read-only and allocation-free. The generated tables remain the source
of truth; this layer does not invent gameplay rules or reinterpret unresolved event
command IDs.

## Verification boundary

A successful data validation confirms internal index/range consistency only. It does
not prove Construct event equivalence, combat, AI, world generation, UI behavior,
audio trigger behavior, or save compatibility. Those remain separate Stage 1 port
and regression tasks, as recorded in `STAGE1_STATUS.md`.
