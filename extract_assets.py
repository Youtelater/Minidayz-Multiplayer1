import os
import py7zr

def extract_game_assets():
    archive_name = "www.7z"
    output_folder = "extracted_game_files"

    if not os.path.exists(archive_name):
        print(f"Error: Could not find {archive_name} in this directory!")
        return

    print(f"Found {archive_name}. Extracting contents...")
    
    # Extract the 7z archive automatically
    with py7zr.SevenZipFile(archive_name, mode='r') as archive:
        archive.extractall(path=output_folder)
        
    print(f"Successfully extracted all files into the '{output_folder}' directory!")
    print("You can now browse the unpacked sprites, audio files, and scripts.")

if __name__ == "__main__":
    extract_game_assets()
