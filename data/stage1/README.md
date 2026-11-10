# Stage 1 extracted data

The complete parsed Construct project snapshot is stored in `source_snapshot/` as ordered text chunks. Run `python tools/stage1/reassemble_snapshot.py` to reconstruct `construct_project_parsed.json`. The extracted inventories and generated C++17 tables are included alongside the snapshot.

The supplied Construct export remains the authoritative extraction source. The repository's `romfs/` directory may be inspected as outdated legacy reference material only; nothing in it is trusted without cross-checking. `raw/` and `c2injt.js` are not extraction sources. Files ending in `mp.json` and non-JSON multiplayer material remain excluded from Stage 1. Original media assets are not modified.
