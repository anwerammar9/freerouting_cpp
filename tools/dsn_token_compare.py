#!/usr/bin/env python3
"""Compare two DSN token dumps (Java reference vs C++ dsn_token_dump).

Usage: dsn_token_compare.py <java.tok> <cpp.tok>
Doubles (D:) are compared numerically, everything else textually. Prints the
first difference and exits 1, or exits 0 when the streams are equal.
"""
import sys


def same(a, b):
    if a == b:
        return True
    if a.startswith("D:") and b.startswith("D:"):
        x, y = float(a[2:]), float(b[2:])
        return x == y or abs(x - y) <= 1e-12 * max(abs(x), abs(y))
    return False


def main():
    with open(sys.argv[1], encoding="utf-8", errors="replace") as f:
        java = f.read().split("\n")
    with open(sys.argv[2], encoding="utf-8", errors="replace") as f:
        cpp = f.read().split("\n")
    for i in range(max(len(java), len(cpp))):
        a = java[i] if i < len(java) else "<eof>"
        b = cpp[i] if i < len(cpp) else "<eof>"
        if not same(a, b):
            print(f"token {i + 1}: java={a!r} cpp={b!r}")
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
