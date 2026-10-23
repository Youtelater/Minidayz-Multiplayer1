import xml.etree.ElementTree as ET
import json
import os

os.makedirs("romfs/data", exist_ok=True)

try:
    # Read the player log XML file
    tree = ET.parse("l_eng_log.xml")
    root = tree.getroot()
    log_map = {}

    # Extract IDs and status messages (e.g., "I'm freezing!", "I feel healthy.")
    for text_node in root.findall('text'):
        text_id = text_node.find('id').text
        text_name = text_node.find('name').text
        if text_id and text_name:
            log_map[text_id] = text_name

    # Save as JSON inside romfs/data/
    output_path = "romfs/data/game_logs.json"
    with open(output_path, "w", encoding="utf-8") as out:
        json.dump(log_map, out, indent=4)

    print(f"Success! Created: {output_path}")

except FileNotFoundError:
    print("Error: Could not find 'l_eng_log.xml'. Make sure it's in the folder!")
