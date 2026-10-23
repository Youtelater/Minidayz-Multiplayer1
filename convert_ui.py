import xml.etree.ElementTree as ET
import json

# This reads your menu words file
tree = ET.parse("l_eng_ui.xml")
root = tree.getroot()
ui_map = {}

# This extracts the IDs and menu button names (like "New Game")
for text_node in root.findall('text'):
    text_id = text_node.find('id').text
    text_name = text_node.find('name').text
    if text_id and text_name:
        ui_map[text_id] = text_name

# This saves it as a JSON file inside your romfs folder
with open("romfs/data/ui_strings.json", "w", encoding="utf-8") as out:
    json.dump(ui_map, out, indent=4)

print("Done! Menu text converted.")
