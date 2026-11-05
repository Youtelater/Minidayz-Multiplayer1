import msgpack
import zlib
import re
import os
import time

input_js_path = "data.deobfuscated.js"
output_bin_path = "romfs/game.bin"
part1_js = "data_part1.js"
part2_js = "data_part2.js"
part1_bin = "part1.bin"
part2_bin = "part2.bin"

if not os.path.exists(input_js_path):
    print(f"[ERROR] Could not find {input_js_path}!")
    exit(1)

print("[1/6] Reading original data.deobfuscated.js...")
with open(input_js_path, "r", encoding="utf-8") as f:
    js_content = f.read()

print("[2/6] Cleaning JavaScript wrapper and splitting 50/50 into two files...")
cleaned_js = re.sub(r'^\s*(var|let|const|window\.)\s*[\w\d_]+\s*=\s*', '', js_content.strip())
if cleaned_js.endswith(';'):
    cleaned_js = cleaned_js[:-1]

inner_content = cleaned_js.strip()
if inner_content.startswith('{') and inner_content.endswith('}'):
    inner_content = inner_content[1:-1].strip()

lines = inner_content.splitlines()
mid_point = len(lines) // 2
part1_lines = lines[:mid_point]
part2_lines = lines[mid_point:]

# Write the 50/50 split parts
with open(part1_js, "w", encoding="utf-8") as f:
    f.write("{\n" + "\n".join(part1_lines) + "\n}")
with open(part2_js, "w", encoding="utf-8") as f:
    f.write("{\n" + "\n".join(part2_lines) + "\n}")

print(f"      -> Created {part1_js} and {part2_js} (50/50 split)")

def process_part(js_filepath, bin_filepath):
    print(f"      -> Evaluating {js_filepath}...")
    with open(js_filepath, "r", encoding="utf-8") as f:
        content = f.read()
    namespace = {}
    exec(f"chunk_data = {content}", namespace)
    chunk_data = namespace['chunk_data']
    
    print(f"      -> Converting and compressing into binary ({bin_filepath})...")
    packed = msgpack.packb(chunk_data)
    compressed = zlib.compress(packed, level=9)
    with open(bin_filepath, "wb") as f:
        f.write(compressed)
    return chunk_data

print("[3/6] Processing Part 1 into binary...")
data1 = process_part(part1_js, part1_bin)

print("[4/6] Processing Part 2 into binary...")
data2 = process_part(part2_js, part2_bin)

print("[5/6] Re-merging data chunks and creating final romfs/game.bin...")
merged_data = {**data1, **data2}

final_packed = msgpack.packb(merged_data)
final_compressed = zlib.compress(final_packed, level=9)

os.makedirs(os.path.dirname(output_bin_path), exist_ok=True)
with open(output_bin_path, "wb") as f:
    f.write(final_compressed)

print("[6/6] Cleaning up temporary files...")
for temp_file in [part1_js, part2_js, part1_bin, part2_bin]:
    if os.path.exists(temp_file):
        os.remove(temp_file)

print(f"\nSuccess! Final merged game.bin size: {len(final_compressed)} bytes")
