import json

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

maps_found = []

def search_maps(obj):
    if isinstance(obj, dict):
        for k, v in obj.items():
            if k == "layers" or "tilemap" in str(v).lower():
                maps_found.append(obj)
            search_maps(v)
    elif isinstance(obj, list):
        for item in obj:
            search_maps(item)

print("Scanning for map layouts and tile data...")
search_maps(game_data)

print(f"Found {len(maps_found)} map/tilemap structures!")

# Save to file
output_file = "extracted_maps.json"
with open(output_file, "w", encoding="utf-8") as out:
    json.dump(maps_found, out, indent=2)

print(f"Successfully saved map data to {output_file}!")