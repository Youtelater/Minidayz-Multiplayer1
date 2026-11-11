import json
import os

# 1. Load your combined raw data
p1_path = os.path.join('romfs', 'data_part_1.json')
p2_path = os.path.join('romfs', 'data_part_2.json')

print("Reading and combining fragments on PC...")
with open(p1_path, 'r', encoding='utf-8') as f:
    part1_str = f.read()
with open(p2_path, 'r', encoding='utf-8') as f:
    part2_str = f.read()

full_json_str = part1_str + part2_str
print("Parsing full JSON dataset...")
full_data = json.loads(full_json_str)

# 2. Create output directory
output_dir = os.path.join('romfs', 'split_data')
os.makedirs(output_dir, exist_ok=True)

# 3. If it's a giant list, chunk it into smaller files of 500 items each
if isinstance(full_data, list):
    chunk_size = 500  # Adjust this number if needed
    for i in range(0, len(full_data), chunk_size):
        chunk = full_data[i:i + chunk_size]
        chunk_path = os.path.join(output_dir, f"project_chunk_{i // chunk_size}.json")
        print(f"Exporting chunk {i // chunk_size} to {chunk_path}...")
        with open(chunk_path, 'w', encoding='utf-8') as out_f:
            json.dump(chunk, out_f)
    print("\n[SUCCESS] Successfully chunked the list into smaller files!")

# If it's a dictionary with a massive nested list inside 'project'
elif isinstance(full_data, dict) and "project" in full_data:
    proj_data = full_data["project"]
    if isinstance(proj_data, list):
        chunk_size = 500
        for i in range(0, len(proj_data), chunk_size):
            chunk = proj_data[i:i + chunk_size]
            chunk_path = os.path.join(output_dir, f"project_chunk_{i // chunk_size}.json")
            print(f"Exporting chunk {i // chunk_size} to {chunk_path}...")
            with open(chunk_path, 'w', encoding='utf-8') as out_f:
                json.dump(chunk, out_f)
        print("\n[SUCCESS] Successfully chunked project data!")
    else:
        # Save normally if it's a manageable dictionary
        with open(os.path.join(output_dir, "project.json"), 'w', encoding='utf-8') as out_f:
            json.dump(proj_data, out_f)
else:
    print("[NOTE] Data structure is customized. Let me know what keys are inside your JSON!")