import os

input_dir = "minidayz_raw"
output_dir = "minidayz_decoded"

os.makedirs(output_dir, exist_ok=True)

for filename in os.listdir(input_dir):
    if filename.endswith(".js"):
        input_path = os.path.join(input_dir, filename)
        output_path = os.path.join(output_dir, filename)
        
        try:
            with open(input_path, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()
            
            decoded_content = content.encode().decode('unicode-escape', errors='ignore')
            
            # Added errors='replace' to prevent surrogate crashes
            with open(output_path, "w", encoding="utf-8", errors="replace") as f:
                f.write(decoded_content)
            print(f"[DECODED] {filename}")
        except Exception as e:
            print(f"[ERROR] Skipping {filename}: {e}")

print("Step 1 complete!")
