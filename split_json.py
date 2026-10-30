import json

# Load your large extracted JSON file
with open("romfs/extracted_maps.json", "r", encoding="utf-8") as f:
    data = json.load(f)

# Figure out the midpoint
midpoint = len(data) // 2

# Split the data into two halves
part1 = data[:midpoint]
part2 = data[midpoint:]

# Save them as separate smaller files
with open("romfs/extracted_maps_part1.json", "w", encoding="utf-8") as f:
    json.dump(part1, f, separators=(',', ':'))

with open("romfs/extracted_maps_part2.json", "w", encoding="utf-8") as f:
    json.dump(part2, f, separators=(',', ':'))

print(f"Successfully split {len(data)} items into two parts ({len(part1)} and {len(part2)})!")