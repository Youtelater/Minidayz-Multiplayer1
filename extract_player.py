import os
import re

def extract_player_data():
    # Path to your extracted js file (adjust name if it's inside a subfolder)
    file_path = os.path.join("www", "mp_players.js")
    
    if not os.path.exists(file_path):
        print(f"Error: Could not find {file_path}. Check your folder structure!")
        return

    print(f"Scanning {file_path} for player stats and data...\n")
    
    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    # Example: Search for player properties using regular expressions
    # This looks for variables or keys related to health, speed, blood, etc.
    patterns = [
        r"health\s*[:=]\s*[\d.]+",
        r"blood\s*[:=]\s*[\d.]+",
        r"speed\s*[:=]\s*[\d.]+",
        r"inventory\s*[:=]\s*\[.*?\]"
    ]

    for pattern in patterns:
        matches = re.findall(pattern, content, re.IGNORECASE)
        if matches:
            print(f"Found matches for pattern '{pattern}':")
            for match in matches[:5]:  # Show first 5 matches to keep it clean
                print(f"  - {match}")
            print()

if __name__ == "__main__":
    extract_player_data()
