#!/usr/bin/env python3
"""Compare `freerouting --dump-board-counts` with the Java oracle JSON of every test board.

Usage: tools/dsn_counts_compare.py <freerouting binary> <repo root> <scratch dir> [--verbose]

For every tests/data/expected/<rel>.counts.json (written by tools/java_ref/BoardCounts.java, T007)
the C++ dump of tests/data/<rel> is compared on result, layers, counts, clearance classes,
clearance matrix and (unless the oracle omitted them) the full item list in item-list order.
The file path and the warning count are ignored. Exit code 0 only if every board matches.
"""
import glob
import json
import os
import subprocess
import sys


def compare(binary, root, scratch, verbose):
    expected_dir = os.path.join(root, "tests", "data", "expected")
    os.makedirs(scratch, exist_ok=True)
    rows = []
    for expected_path in sorted(glob.glob(expected_dir + "/**/*.counts.json", recursive=True)):
        rel = os.path.relpath(expected_path, expected_dir)[: -len(".counts.json")]
        dsn = os.path.join(root, "tests", "data", rel)
        out = os.path.join(scratch, rel.replace("/", "__") + ".json")
        try:
            subprocess.run([binary, "--dump-board-counts", dsn, out], stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL, timeout=120, check=False)
            actual = json.load(open(out, encoding="utf-8"))
        except Exception as error:  # timeout, crash, unreadable output
            rows.append((rel, "no output: " + str(error)[:100]))
            continue
        expected = json.load(open(expected_path, encoding="utf-8"))
        diffs = []
        for key in ["result", "layers", "counts", "clearance_classes", "clearance_matrix"]:
            if expected.get(key) != actual.get(key):
                if key == "counts" and isinstance(actual.get(key), dict):
                    diffs.append("counts " + ",".join(
                        f"{k}={actual[key].get(k)}/{v}" for k, v in expected[key].items() if actual[key].get(k) != v))
                else:
                    diffs.append(f"{key} {actual.get(key)}/{expected.get(key)}" if key == "result" else key)
        if not expected.get("items_omitted") and expected.get("items") != actual.get("items"):
            exp_items, act_items = expected.get("items", []), actual.get("items", [])
            first = next((i for i, (a, b) in enumerate(zip(exp_items, act_items)) if a != b),
                         min(len(exp_items), len(act_items)))
            diffs.append(f"items (len {len(act_items)}/{len(exp_items)}, first difference at #{first})")
        rows.append((rel, "OK" if not diffs else "; ".join(diffs)))
    matched = sum(1 for _, status in rows if status == "OK")
    for rel, status in rows:
        if verbose or status != "OK":
            print(f"{status[:200]:<10}  {rel}")
    print(f"{matched}/{len(rows)} boards match the Java oracle")
    return 0 if rows and matched == len(rows) else 1


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(__doc__)
        sys.exit(2)
    sys.exit(compare(sys.argv[1], sys.argv[2], sys.argv[3], "--verbose" in sys.argv[4:]))
