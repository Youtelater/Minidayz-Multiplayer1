import json
import msgpack
import zlib
import re

input_js_path = "data.deobfuscated.js" # Change to your actual file name
output_bin_path = "romfs/game.bin"

print("Reading original data.js file...")
with open(input_js_path, "r", encoding="utf-8") as f:
    js_content = f.read()

# Step 1: Strip the JavaScript variable wrapper (e.g., "var data = " or "export default")
# This looks for the first opening brace '{' and the last closing brace '}'
match = re.search(r'(\{[\s\S]*\})', js_content)
if not match:
    raise ValueError("Could not find a valid JSON object block inside the .js file!")

json_string = match.group(1)

print("Parsing JSON from JavaScript content...")
raw_data = json.loads(json_string)

print("Packing into MessagePack binary...")
packed_data = msgpack.packb(raw_data)

print("Compressing with zlib...")
compressed_data = zlib.compress(packed_data, level=9)

print(f"Writing compressed binary to {output_bin_path}...")
with open(output_bin_path, "wb") as f:
    f.write(compressed_data)

print(f"Done! Compressed size: {len(compressed_data)} bytes")
