import xml.etree.ElementTree as ET
import json
import os

# Make sure the romfs/data folder exists before saving
os.makedirs("romfs/data", exist_ok=True)

try:
    # Read your menu XML file
    tree = ET.parse("l_eng_ui.xml")
    root = tree.getroot()
    ui_map = {}

    # Extract IDs and names
    for text_node in root.findall('text'):
        text_id = text_node.find('id').text
        text_name = text_node.find('name').text
        if text_id and text_name:
            ui_map[text_id] = text_name

    # Save as JSON inside romfs/data/
    output_path = "romfs/data/ui_strings.json"
    with open(output_path, "w", encoding="utf-8") as out:
        json.dump(ui_map, out, indent=4)

    print(f"Success! Created: {output_path}")

except FileNotFoundError:
    print("Error: Could not find 'l_eng_ui.xml'. Make sure it is in the same folder as this script!")
