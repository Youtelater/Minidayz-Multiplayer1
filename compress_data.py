import json
import msgpack
import zlib
import os

print("Reading clean_data.json...")
with open("clean_data.json", "r", encoding="utf-8") as f:
    raw_data = json.load(f)

print("Packing data to MessagePack...")
packed = msgpack.packb(raw_data)

print("Compressing with zlib (level 9)...")
compressed = zlib.compress(packed, level=9)

os.makedirs("romfs", exist_ok=True)
with open("romfs/game.bin", "wb") as f:
    f.write(compressed)

print(f"Success! romfs/game.bin created ({len(compressed)} bytes).")
