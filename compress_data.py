import json
import msgpack
import zlib

# Path to your original, un-separated JSON file
input_json_path = "data.deobfuscated.js" 
output_bin_path = "romfs/game.bin"

print("Reading original JSON file...")
with open(input_json_path, "r", encoding="utf-8") as f:
    raw_data = json.load(f)

print("Packing into MessagePack binary...")
packed_data = msgpack.packb(raw_data)

print("Compressing with zlib...")
compressed_data = zlib.compress(packed_data, level=9)

print(f"Writing compressed binary to {output_bin_path}...")
with open(output_bin_path, "wb") as f:
    f.write(compressed_data)

print(f"Done! Original size: {len(packed_data)} bytes | Compressed size: {len(compressed_data)} bytes")
