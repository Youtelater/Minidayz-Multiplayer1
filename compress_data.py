import msgpack
import zlib
import re
import os
import time

input_js_path = "data.deobfuscated.js"
output_bin_path = "romfs/game.bin"

if not os.path.exists(input_js_path):
    print(f"[ERROR] Could not find {input_js_path}. Make sure it's in the same folder!")
    exit(1)

start_time = time.time()

print(f"[1/5] Reading {input_js_path} (this can take a few seconds for large files)...")
with open(input_js_path, "r", encoding="utf-8") as f:
    js_content = f.read()
print(f"      -> Read {len(js_content)} characters in {time.time() - start_time:.2f}s")

print("[2/5] Cleaning JavaScript wrapper...")
cleaned_js = re.sub(r'^\s*(var|let|const|window\.)\s*[\w\d_]+\s*=\s*', '', js_content.strip())
if cleaned_js.endswith(';'):
    cleaned_js = cleaned_js[:-1]

print("[3/5] Evaluating JavaScript object structure safely (please wait)...")
eval_start = time.time()
global_namespace = {}
try:
    exec(f"raw_data = {cleaned_js}", global_namespace)
    raw_data = global_namespace['raw_data']
except Exception as e:
    print(f"[ERROR] Failed to parse JavaScript content: {e}")
    exit(1)
print(f"      -> Evaluated in {time.time() - eval_start:.2f}s")

print("[4/5] Packing data into MessagePack binary...")
pack_start = time.time()
packed_data = msgpack.packb(raw_data)
print(f"      -> Packed {len(packed_data)} bytes in {time.time() - pack_start:.2f}s")

print("[5/5] Compress data with zlib (level 9)...")
compress_start = time.time()
compressed_data = zlib.compress(packed_data, level=9)
print(f"      -> Compressed to {len(compressed_data)} bytes in {time.time() - compress_start:.2f}s")

os.makedirs(os.path.dirname(output_bin_path), exist_ok=True)
with open(output_bin_path, "wb") as f:
    f.write(compressed_data)

print(f"\nDone! Successfully wrote to {output_bin_path} in {time.time() - start_time:.2f} seconds total.")
