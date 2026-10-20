import re
import json

def extract_strings_from_js(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as file:
        content = file.read()

    # Regular expression to match single or double-quoted string literals
    pattern = r'"([^"\\]*(?:\\.[^"\\]*)*)"|\'([^\'\\]*(?:\\.[^\'\\]*)*)\''
    matches = re.findall(pattern, content)
    
    extracted_strings = []
    for match in matches:
        raw_str = match[0] if match[0] else match[1]
        if raw_str.strip():
            try:
                # Automatically decode \uXXXX sequences into readable characters
                decoded_str = raw_str.encode().decode('unicode-escape')
                extracted_strings.append(decoded_str)
            except Exception:
                # Fallback to raw string if decoding fails
                extracted_strings.append(raw_str)

    return extracted_strings

if __name__ == "__main__":
    js_file_path = "mp_menu.js"  # Make sure this matches your JS file name
    strings = extract_strings_from_js(js_file_path)
    
    print(f"Extracted and decoded {len(strings)} strings:")
    for s in strings[:30]:  # Print a preview of the first 30 decoded strings
        print(s)
        
    # Save the decoded strings to a clean JSON file
    with open("decoded_strings.json", "w", encoding="utf-8") as outfile:
        json.dump(strings, outfile, indent=4, ensure_ascii=False)
    print("\nSaved decoded strings to decoded_strings.json!")
