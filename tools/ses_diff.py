#!/usr/bin/env python3
"""Token-level comparison of two Specctra session (.ses) files.

Usage: tools/ses_diff.py a.ses b.ses [--eps 0]

Both files are tokenized into parentheses, quoted strings and atoms (layout and indentation are
ignored). Numeric atoms are equal if they differ by at most --eps (default 0). Prints the scope path
of the first difference (e.g. session/routes/network_out/net "GND"/wire/path) and exits with 1, or
prints "equal" and exits with 0.
"""
import argparse
import re
import sys

TOKEN_RE = re.compile(r'\(|\)|"(?:[^"\\]|\\.)*"|[^\s()"]+')


def tokenize(path):
    with open(path, encoding="utf-8", errors="replace") as file:
        return TOKEN_RE.findall(file.read())


def atoms_equal(a, b, eps):
    if a == b:
        return True
    try:
        return abs(float(a) - float(b)) <= eps
    except ValueError:
        return False


def scope_path(tokens, index):
    """The keywords (plus a name argument) of the scopes open at token index."""
    stack = []
    i = 0
    while i < index:
        token = tokens[i]
        if token == "(":
            label = tokens[i + 1] if i + 1 < len(tokens) else "?"
            if i + 2 < len(tokens) and tokens[i + 2] not in ("(", ")"):
                label += " " + tokens[i + 2]
            stack.append(label)
        elif token == ")" and stack:
            stack.pop()
        i += 1
    return "/".join(stack)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("first")
    parser.add_argument("second")
    parser.add_argument("--eps", type=float, default=0.0)
    args = parser.parse_args()
    first, second = tokenize(args.first), tokenize(args.second)
    for index in range(min(len(first), len(second))):
        if not atoms_equal(first[index], second[index], args.eps):
            print(f"differ at token {index} in {scope_path(first, index)}: {first[index]!r} vs {second[index]!r}")
            return 1
    if len(first) != len(second):
        print(f"differ in length: {len(first)} vs {len(second)} tokens "
              f"(after {scope_path(first, min(len(first), len(second)))})")
        return 1
    print("equal")
    return 0


if __name__ == "__main__":
    sys.exit(main())
