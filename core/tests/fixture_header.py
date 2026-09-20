"""Compile shared website/firmware selection expectations into a native test header."""

import json
import sys
from pathlib import Path

data = json.loads(Path(sys.argv[1]).read_text())
forms = ["icebreaker", "reflective", "hypothetical", "memory", "would-you-rather"]
rows = []
for question in data["questions"]:
    tags = question["tags"]
    permissions = int("dark" in tags) | (int("sexual" in tags) << 1)
    form_bits = sum(1 << i for i, tag in enumerate(forms) if tag in tags)
    rows.append(f"{{{question['depth']}, {permissions}, {form_bits}}}")
text = "#pragma once\nnamespace shared_fixture {\n"
text += "constexpr unsigned questions[][3] = {" + ",".join(rows) + "};\n"
for name in ["permissions", "draws"]:
    text += f"constexpr unsigned {name}[] = {{" + ",".join(map(str, data[name])) + "};\n"
eligible = [sum(1 << i for i in indices) for indices in data["eligible"]]
text += "constexpr unsigned eligible[] = {" + ",".join(map(str, eligible)) + "};\n"
text += f"constexpr unsigned seen = {sum(1 << i for i in data['seen'])};\n}}\n"
Path(sys.argv[2]).write_text(text)
