import msgpack
import zlib
import re
import os

input_js_path = "data.deobfuscated.js"
output_bin_path = "romfs/game.bin"

if not os.path.exists(input_js_path):
    print(f"[ERROR] Could not find {input_js_path}. Make sure it's in the same folder!")
    exit(1)

print(f"Reading {input_js_path}...")
with open(input_js_path, "r", encoding="utf-8") as f:
    js_content = f.read()

# Strip any variable declarations (e.g., "var gamedata = " or "window.data =") 
# leaving just the raw object assignment
cleaned_js = re.sub(r'^\s*(var|let|const|window\.)\s*[\w\d_]+\s*=\s*', '', js_content.strip())
if cleaned_js.endswith(';'):
    cleaned_js = cleaned_js[:-1]

print("Evaluating JavaScript object structure safely...")
global_namespace = {}
try:
    exec(f"raw_data = {cleaned_js}", global_namespace)
    raw_data = global_namespace['raw_data']
except Exception as e:
    print(f"[ERROR] Failed to parse JavaScript content: {e}")
    exit(1)

print("Packing data into MessagePack binary...")
packed_data = msgpack.packb(raw_data)

print("Compressing data with zlib...")
compressed_data = zlib.compress(packed_data, level=9)

# Ensure the output romfs directory exists
os.makedirs(os.path.dirname(output_bin_path), exist_ok=True)

print(f"Writing compressed binary to {output_bin_path}...")
with open(output_bin_path, "wb") as f:
    f.write(compressed_data)

print(f"Success! Unpacked MessagePack size: {len(packed_data)} bytes | Compressed game.bin size: {len(compressed_data)} bytes")
