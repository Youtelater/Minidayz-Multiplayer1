import re
import json

# Example: Read your original mp_players.js or config file
js_file_path = "mp_players.js"
output_json_path = "romfs/data/player_data.json"

try:
    with open(js_file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Use regex to find key-value data structures inside the original JS code
    # (Adjust this pattern based on how data is structured in your specific .js files)
    extracted_data = {}
    
    # Example pattern looking for player attributes or stats
    matches = re.findall(r'(\w+):\s*([0-9\.]+)', content)
    for key, val in matches:
        extracted_data[key] = float(val) if '.' in val else int(val)

    # Save out as clean JSON for your C++ 3DS homebrew
    with open(output_json_path, "w", encoding="utf-8") as out_f:
        json.dump(extracted_data, out_f, indent=4)
        
    print("Successfully converted JS data to JSON for RomFS!")

except FileNotFoundError:
    print(f"Could not find {js_file_path}. Make sure it's in the same directory.")
