import os
import json

def convert_to_json(input_dir, output_dir):
    os.makedirs(output_dir, exist_ok=True)
    for filename in os.listdir(input_dir):
        if filename.endswith(".js"):
            in_path = os.path.join(input_dir, filename)
            base_name = os.path.splitext(filename)[0]
            out_path = os.path.join(output_dir, f"{base_name}.json")
            
            with open(in_path, 'r', encoding='utf-8') as f:
                content = f.read()
                
            # Strip JS wrapper and grab inner braces
            start_idx = content.find('{')
            end_idx = content.rfind('}')
            
            if start_idx != -1 and end_idx != -1:
                json_str = content[start_idx:end_idx+1]
                try:
                    data = json.loads(json_str)
                    with open(out_path, 'w', encoding='utf-8') as jf:
                        json.dump(data, jf, indent=2, ensure_ascii=False)
                    print(f"[JSON] Converted {filename}")
                except Exception as e:
                    print(f"[ERROR] Could not parse JSON for {filename}: {e}")
                    
    print("\n--- Step 2 Complete: Clean JSON files generated. ---")

convert_to_json("minidayz_decoded", "minidayz_json")
