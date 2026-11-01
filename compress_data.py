import json
import os
import zlib
import struct

# Define paths to your split fragments inside romfs
p1_path = os.path.join('romfs', 'data_part_1.json')
p2_path = os.path.join('romfs', 'data_part_2.json')

print("Reading JSON parts...")
with open(p1_path, 'r', encoding='utf-8') as f:
    part1_str = f.read()
with open(p2_path, 'r', encoding='utf-8') as f:
    part2_str = f.read()

full_json_str = part1_str + part2_str

print("Compressing data into binary format...")
json_bytes = full_json_str.encode('utf-8')
compressed_data = zlib.compress(json_bytes, level=9) # Maximum compression

print(f"Original size: {len(json_bytes) / (1024*1024):.2f} MB")
print(f"Compressed binary size: {len(compressed_data) / (1024*1024):.2f} MB")

# Save as a clean binary file with a size header inside romfs
output_path = os.path.join('romfs', 'game_compressed.bin')
with open(output_path, 'wb') as f:
    f.write(struct.pack('<I', len(compressed_data)))
    f.write(compressed_data)

print(f"Successfully created compressed binary file at: {output_path}")
