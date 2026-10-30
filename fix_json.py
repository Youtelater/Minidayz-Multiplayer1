import ast
import json
import re

print("Reading combined file...")
with open("full_data.js", "r", encoding="utf-8") as f:
    content = f.read()

# Find the start of the actual data array or object
start_brace = content.find('{')
start_bracket = content.find('[')

if start_brace == -1 and start_bracket == -1:
    print("Error: Could not find data structure.")
    exit(1)

start_idx = min(i for i in [start_brace, start_bracket] if i != -1)
end_idx = max(content.rfind('}'), content.rfind(']'))
data_str = content[start_idx:end_idx+1]

print("Cleaning JavaScript syntax for Python evaluation...")

# 1. Replace JS true/false/null with Python equivalents
data_str = data_str.replace('true', 'True').replace('false', 'False').replace('null', 'None')

# 2. Fix negative numbers with spaces after the minus sign (e.g., "- 0.16" -> "-0.16")
data_str = re.sub(r'-\s+(\d)', r'-\1', data_str)

print("Evaluating data structure (this may take a moment)...")
try:
    # Safely parse the giant JS/Python-like literal structure
    parsed_data = ast.literal_eval(data_str)
    
    print("Parsing successful! Saving to data.json...")
    with open("data.json", "w", encoding="utf-8") as out:
        json.dump(parsed_data, out, indent=2)
        
    print("Success! Cleaned data saved to 'data.json'")

except Exception as e:
    print(f"Literal evaluation error: {e}")
