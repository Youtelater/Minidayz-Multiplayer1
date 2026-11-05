import msgpack
import zlib
import re
import os
import time

input_js_path = "data.deobfuscated.js"
output_bin_path = "romfs/game.bin"

if not os.path.exists(input_js_path):
    print(f"[ERROR] Could not find {input_js_path}!")
    exit(1)

print("[1/4] Reading original data.deobfuscated.js...")
start_time = time.time()
with open(input_js_path, "r", encoding="utf-8") as f:
    js_content = f.read()

print("[2/4] Cleaning JavaScript wrapper...")
cleaned_js = re.sub(r'^\s*(var|let|const|window\.)\s*[\w\d_]+\s*=\s*', '', js_content.strip())
if cleaned_js.endswith(';'):
    cleaned_js = cleaned_js[:-1]

print("[3/4] Evaluating JavaScript object safely...")
# Using a restricted global namespace to speed up evaluation
global_namespace = {}
try:
    exec(f"raw_data = {cleaned_js}", global_namespace)
    raw_data = global_namespace['raw_data']
    print(f"      -> Evaluated successfully in {time.time() - start_time:.2f}s")
except Exception as e:
    print(f"[ERROR] Failed to parse JavaScript content: {e}")
    exit(1)

print("[4/4] Packing into MessagePack and compressing with zlib...")
packed_data = msgpack.packb(raw_data)
compressed_data = zlib.compress(packed_data, level=9)

os.makedirs(os.path.dirname(output_bin_path), exist_ok=True)
with open(output_bin_path, "wb") as f:
    f.write(compressed_data)

print(f"\nSuccess! Final compressed game.bin size: {len(compressed_data)} bytes")
