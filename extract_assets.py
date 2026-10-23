import os

def extract_all_player_data():
    file_path = "mp_players.js"
    
    if not os.path.exists(file_path):
        print(f"Error: Could not find '{file_path}' in the root directory!")
        return

    print(f"Scanning '{file_path}' for all player properties and stats...\n")
    
    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()

    # Keywords to look for to catch stats, inventory, movement, and configuration
    keywords = ["health", "blood", "speed", "inventory", "stamina", "position", "damage", "armor", "food", "water"]

    match_count = 0
    for line_num, line in enumerate(lines, 1):
        stripped = line.strip()
        # Print lines that contain any of the player keywords or variable declarations
        if any(kw in stripped.lower() for kw in keywords) or "player" in stripped.lower():
            print(f"[Line {line_num}] {stripped}")
            match_count += 1

    print(f"\nScan complete! Found {match_count} lines matching player data.")

if __name__ == "__main__":
    extract_all_player_data()
