import json
import struct

# 1. Open your large JSON file
with open('data_part_1.json', 'r', encoding='utf-8') as f:
    data = json.load(f)

# 2. Convert it into packed binary bytes
json_str = json.dumps(data)
json_bytes = json_str.encode('utf-8')

# 3. Save it as a new .bin file
with open('data_part_1.bin', 'wb') as f:
    f.write(struct.pack('<I', len(json_bytes)))
    f.write(json_bytes)

print("Done! Created data_part_1.bin")
