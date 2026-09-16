"""Reject assembly exports with unresolved purchasing identifiers."""
import csv
import sys


def check(path):
    with open(path, newline="") as stream:
        reader = csv.DictReader(stream)
        if not {"Refs", "MPN"}.issubset(reader.fieldnames or []):
            raise SystemExit("Assembly BOM must contain Refs and MPN columns")
        rows = list(reader)
    unresolved = []
    for row in rows:
        if not (row.get("Refs") or "").strip():
            raise SystemExit("Assembly BOM has a row without component references")
        mpn = (row.get("MPN") or "").strip()
        if not mpn or any(token in mpn.lower() for token in [";", " alt ", "custom", "tbd"]):
            unresolved.append(row["Refs"])
    if unresolved:
        raise SystemExit("Assembly BOM has no single selected MPN for: " + ", ".join(unresolved))
    if not rows:
        raise SystemExit("Assembly BOM is empty")


if __name__ == "__main__":
    check(sys.argv[1])
