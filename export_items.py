import json
import os

print("Reading data.js...")
with open("data.js", "r", encoding="utf-8-sig") as f:
    raw_content = f.read()

# Clean JavaScript wrapper if present
if raw_content.startswith("var ") or raw_content.startswith("window."):
    first_brace = raw_content.find("{")
    first_bracket = raw_content.find("[")
    start_idx = min(first_brace, first_bracket) if (first_brace != -1 and first_bracket != -1) else max(first_brace, first_bracket)
    if start_idx != -1:
        raw_content = raw_content[start_idx:]

print("Parsing JSON structure...")
game_data = json.loads(raw_content)

# Let's search for weapon/item references and save them
weapons = []
search_keywords = ["rifle", "smg", "helmet", "pants", "armor", "ammo"]

# Convert the whole data structure to a string representation or search recursively
# To make it robust, let's write out a search function over lists/dicts
def search_data(obj):
    if isinstance(obj, dict):
        for k, v in obj.items():
            search_data(v)
    elif isinstance(obj, list):
        for item in obj:
            if isinstance(item, str) and any(kw in item.lower() for kw in search_keywords):
                weapons.append(item)
            else:
                search_data(item)

print("Scanning for weapons and equipment...")
search_data(game_data)

# Remove duplicates
unique_weapons = list(set(weapons))
print(f"Found {len(unique_weapons)} weapon/gear references!")

# Save them to a clean file for your C++ engine
output_file = "extracted_weapons.json"
with open(output_file, "w", encoding="utf-8") as out:
    json.dump(unique_weapons, out, indent=2)

print(f"Successfully saved clean list to {output_file}!")