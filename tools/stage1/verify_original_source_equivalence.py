#!/usr/bin/env python3
"""Compare the two original Construct exports structurally, not by bytes.

Usage: python3 tools/stage1/verify_original_source_equivalence.py
Whitespace and key ordering are irrelevant; the first structural difference
is reported with its JSON path. Never silently overwrite either source.
"""
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
FIXED_SPACED_NEGATIVE = re.compile(r"(?<![A-Za-z0-9_.])-\s+(?=\d)")


def load(path):
    text = path.read_text(encoding="utf-8-sig")
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        # Preserve the same source-recovery rule as extract_construct.py.
        return json.loads(FIXED_SPACED_NEGATIVE.sub("-", text))


def first_difference(a, b, path="$"):
    if type(a) is not type(b):
        return path, type(a).__name__, type(b).__name__
    if isinstance(a, dict):
        ak, bk = set(a), set(b)
        if ak != bk:
            return path + ".keys", sorted(ak - bk)[:10], sorted(bk - ak)[:10]
        for key in sorted(ak):
            found = first_difference(a[key], b[key], path + "[" + repr(key) + "]")
            if found is not None:
                return found
    elif isinstance(a, list):
        if len(a) != len(b):
            return path + ".length", len(a), len(b)
        for i, (left, right) in enumerate(zip(a, b)):
            found = first_difference(left, right, path + "[" + str(i) + "]")
            if found is not None:
                return found
    elif a != b:
        return path, repr(a)[:160], repr(b)[:160]
    return None


def main():
    left_path = ROOT / "data.js"
    right_path = ROOT / "data.deobfuscated.js"
    left, right = load(left_path), load(right_path)
    mismatch = first_difference(left, right)
    print("Original source semantic comparison:")
    print(f"  {left_path.name}: {left_path.stat().st_size:,} bytes")
    print(f"  {right_path.name}: {right_path.stat().st_size:,} bytes")
    print("  data.js project keys:", list(left.keys()) if isinstance(left, dict) else "not object")
    print("  deobfuscated project keys:", list(right.keys()) if isinstance(right, dict) else "not object")
    if mismatch:
        print("RESULT: DIFFERENT (first structural difference)")
        print("  JSON path:", mismatch[0])
        print("  data.js:", mismatch[1])
        print("  data.deobfuscated.js:", mismatch[2])
        return 1
    print("RESULT: EQUIVALENT (all parsed JSON values match)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
