import json
import os
import zlib
import msgpack

# Define paths to your split fragments inside romfs
p1_path = os.path.join('romfs', 'data_part_1.json')
p2_path = os.path.join('romfs', 'data_part_2.json')

print("Reading JSON parts...")
with open(p1_path, 'r', encoding='utf-8') as f:
    part1_str = f.read()
with open(p2_path, 'r', encoding='utf-8') as f:
    part2_str = f.read()

# 1. Combine the JSON string fragments
full_json_str = part1_str + part2_str

print("Parsing combined JSON...")
# 2. Parse into a standard Python data structure
data_obj = json.loads(full_json_str)

print("Packing into MessagePack format...")
# 3. Convert Python object to MessagePack binary format (matches C++ msgpack::unpack)
packed_data = msgpack.packb(data_obj)

print("Compressing with zlib...")
# 4. Compress the binary payload using zlib
compressed_data = zlib.compress(packed_data)

# 5. Write out the final game.bin file into romfs
output_path = os.path.join('romfs', 'game.bin')
with open(output_path, 'wb') as f:
    f.write(compressed_data)

print(f"Successfully generated {output_path} ({len(compressed_data)} bytes)!")
