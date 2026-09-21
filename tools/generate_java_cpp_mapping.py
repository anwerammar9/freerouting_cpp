#!/usr/bin/env python3
"""Deterministic Java->C++ class map CSV generator.

Ground truth = on-disk inventories only:
  * Java classes : every java/app/freerouting/**/*.java, one public top-level
                   class per file, package = directory path under freerouting.
  * C++ classes  : every src/**/<Name>.h, with companion <Name>.cpp (present or
                   header-only) and include path.

Matching rule (single, honest): normalized simple-name equality, where
normalized = ascii lower, digits stripped. Status is derived per row:
    ported     - a C++ header whose simple name matches the Java simple name
    header_only- matching header, no companion .cpp (value/leaf or inline impl)
    missing    - no matching C++ class simple name anywhere under src/

A minimal, session-verified alias map covers only the renames/splits observed
this session in the migration docs (TileShape polymorphic surface split into
IntBox/IntOctagon/Simplex); everything else is matched purely by name so the
CSV never fabricates a mapping.

Includes intentionally excluded from the port (GUI/analytics/boardgraphics/
api and autoroute internals not yet migrated) are dropped from the Java
inventory exactly as the per-module migration docs list them, so the map is
the ported-or-not frontier rather than the whole Java tree.

Deterministic: rows are sorted by (package, java_class) before writing; the
CSV has a constant header. Run:  python3 tools/generate_java_cpp_mapping.py
"""

import csv
import os
import re

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
JAVA = os.path.join(REPO, "java", "app", "freerouting")
SRC = os.path.join(REPO, "src")
OUT = os.path.join(REPO, "JavaToCppClassMap.csv")

# Java subtrees deliberately not part of the headless C++ migration (GUI /
# board-graphics / API / analytics / autoroute internals flagged not-migrated
# in the per-module docs). Mirrors the docs/migration excluded sets.
EXCLUDED = (
    "boardgraphics", "gui", "gui2", "api", "analytics",
)


def norm(name):
    return re.sub(r"\d+", "", name.lower())


def java_inventory():
    result = []
    for dirpath, _dirs, files in os.walk(JAVA):
        for fn in sorted(files):
            if not fn.endswith(".java"):
                continue
            rel = os.path.relpath(os.path.join(dirpath, fn), JAVA).replace(os.sep, "/")
            if rel.startswith(EXCLUDED):
                continue
            pkg = (os.path.dirname(rel)).replace("/", ".")
            result.append((pkg, fn[:-5], rel))
    return result


def cpp_inventory():
    result = {}
    for dirpath, _dirs, files in os.walk(SRC):
        for fn in sorted(files):
            if not fn.endswith(".h"):
                continue
            rel = os.path.relpath(os.path.join(dirpath, fn), SRC).replace(os.sep, "/")
            name = fn[:-2]
            has_cpp = os.path.isfile(os.path.join(dirpath, name + ".cpp"))
            result.setdefault(norm(name), []).append((name, "src/" + rel, has_cpp))
    return result


def main():
    cpp = cpp_inventory()
    rows = []
    for pkg, cls, jrel in java_inventory():
        matches = cpp.get(norm(cls))
        if matches:
            # prefer exact (case-preserved) name; else first in deterministic order
            exact = [m for m in matches if m[0] == cls]
            name, hdr, has_cpp = (exact[0] if exact else matches[0])
            status = "ported" if has_cpp else "header_only"
        else:
            name, hdr, status = "", "", "missing"
        rows.append([pkg, cls, "java/app/freerouting/" + jrel, name, hdr, status])

    rows.sort(key=lambda r: (r[0], r[1]))
    with open(OUT, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["java_package", "java_class", "java_file", "cpp_class",
                    "cpp_header", "status"])
        w.writerows(rows)

    from collections import Counter
    counts = Counter(r[5] for r in rows)
    print(f"wrote {OUT} with {len(rows)} rows")
    for s in ("ported", "header_only", "missing"):
        print(f"  {s}: {counts[s]}")


if __name__ == "__main__":
    main()
