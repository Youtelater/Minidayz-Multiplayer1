import os
import re
import json
import zlib
import msgpack

def process_js_to_bin(input_dir, decoded_json_dir, output_bin_dir):
    os.makedirs(decoded_json_dir, exist_ok=True)
    os.makedirs(output_bin_dir, exist_ok=True)

    for filename in os.listdir(input_dir):
        if filename.endswith(".js"):
            input_path = os.path.join(input_dir, filename)
            base_name = os.path.splitext(filename)[0]
            
            try:
                with open(input_path, 'r', encoding='utf-8') as f:
                    content = f.read()

                # 1. Decode Unicode escape sequences (\uXXXX)
                decoded_content = content.encode().decode('unicode-escape')

                # 2. Find the first opening brace '{' and last closing brace '}' to extract raw JSON/dict data
                start_idx = decoded_content.find('{')
                end_idx = decoded_content.rfind('}')

                if start_idx != -1 and end_idx != -1:
                    json_str = decoded_content[start_idx:end_idx+1]
                    
                    # Parse into Python data structure
                    data = json.loads(json_str)

                    # Save clean decoded JSON file
                    json_output_path = os.path.join(decoded_json_dir, f"{base_name}.json")
                    with open(json_output_path, 'w', encoding='utf-8') as jf:
                        json.dump(data, jf, indent=2, ensure_ascii=False)

                    # 3. Pack into MessagePack and compress with zlib -> .bin
                    packed_data = msgpack.packb(data)
                    compressed_data = zlib.compress(packed_data)

                    bin_output_path = os.path.join(output_bin_dir, f"{base_name}.bin")
                    with open(bin_output_path, 'wb') as bf:
                        bf.write(compressed_data)

                    print(f"[SUCCESS] {filename} processed!")
                else:
                    print(f"[WARNING] Could not find JSON braces in {filename}")

            except Exception as e:
                print(f"[ERROR] Failed processing {filename}: {e}")

# Make sure your input folder matches where your files are located
input_folder = "."  # or "minidayz_raw" if you put them in a folder
json_folder = "minidayz_json"
bin_folder = "romfs_bins"

process_js_to_bin(input_folder, json_folder, bin_folder)
