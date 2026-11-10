# Stage 1 extracted data

The complete parsed Construct project snapshot is stored in `source_snapshot/` as ordered text chunks. Run `python tools/stage1/reassemble_snapshot.py` to reconstruct `construct_project_parsed.json`. The extracted inventories and generated C++17 tables are included alongside the snapshot.

No `romfs/`, `raw/`, `c2injt.js`, or multiplayer JSON files are used as extraction sources. Original media assets are not modified.
