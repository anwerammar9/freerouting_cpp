#!/usr/bin/env python3
"""Validate JavaToCppClassMap.csv against reality.

Checks:
  - every Java file on disk appears exactly once in the CSV (except the
    MazeRandom marker row);
  - statuses are one of ported / header_only / partial / pending / out_of_scope;
  - out_of_scope rows are exactly the explicit OUT_OF_SCOPE table, each with
    its one-word reason;
  - ported / header_only rows have an existing cpp_header declaring the class;
  - partial rows may have an empty cpp_class but a cpp_header must exist.

Prints counts per status and the pending list grouped by package. Exits
non-zero only on structural errors, never on pending > 0.

Usage: python3 tools/class_map_check.py [CSV_PATH] [REPO_ROOT]
"""
import csv
import os
import re
import sys

CSV_PATH = os.environ.get('CLASS_MAP_CSV', '')
REPO_ROOT = os.environ.get('CLASS_MAP_ROOT', '')

if not CSV_PATH:
    CSV_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'JavaToCppClassMap.csv')
if not REPO_ROOT:
    REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

JAVA_ROOT = 'java/app/freerouting'
MAZE_RANDOM_MARKER = 'java.util.Random (JDK)'

# Explicit out-of-scope list: {java path (relative to JAVA_ROOT): one-word reason}.
# Directory rules are expanded by walking the tree; exact files are listed here.
OUT_OF_SCOPE_DIRS = {
    'gui/': 'gui',
    'api/mcp/': 'mcp',
    'api/': 'api',
    'analytics/': 'analytics',
    'io/kicad/': 'kicad',
    'util/gson/': 'gson-api',
}
OUT_OF_SCOPE_FILES = {
    'io/specctra/parser/SessionToFusion.java': 'fusion',
    'io/KiCadNetClassNames.java': 'kicad',
    'util/VersionChecker.java': 'version-check',
    'util/SafeObjectInputStream.java': 'serialization',
    'settings/ApiAuthenticationSettings.java': 'ui-settings',
    'settings/ApiServerSettings.java': 'ui-settings',
    'settings/GuiApplicationSettings.java': 'ui-settings',
    'settings/GoogleSheetsProviderSettings.java': 'ui-settings',
    'settings/McpServerSettings.java': 'ui-settings',
    'settings/RateLimitSettings.java': 'ui-settings',
    'settings/NetworkSettings.java': 'ui-settings',
    'settings/UsageAndDiagnosticDataSettings.java': 'ui-settings',
    'settings/UserProfileSettings.java': 'ui-settings',
    'settings/sources/ApiSettings.java': 'ui-settings',
    'settings/sources/GuiSettingsSource.java': 'ui-settings',
}


def build_out_of_scope():
    table = dict(OUT_OF_SCOPE_FILES)
    for root, dirs, files in os.walk(os.path.join(REPO_ROOT, JAVA_ROOT)):
        rel_root = os.path.relpath(root, os.path.join(REPO_ROOT, JAVA_ROOT))
        for name in files:
            if not name.endswith('.java'):
                continue
            pkg_dir = rel_root.replace(os.sep, '/') + '/'
            for prefix, reason in OUT_OF_SCOPE_DIRS.items():
                if pkg_dir.startswith(prefix):
                    table[os.path.join(rel_root, name).replace(os.sep, '/')] = reason
                    break
    return table


def find_java_files_on_disk():
    files = set()
    for root, dirs, names in os.walk(os.path.join(REPO_ROOT, JAVA_ROOT)):
        for name in names:
            if name.endswith('.java'):
                rel_root = os.path.relpath(root, os.path.join(REPO_ROOT, JAVA_ROOT))
                p = os.path.join(rel_root, name).replace(os.sep, '/')
                if p.startswith('./'):
                    p = p[2:]
                files.add(p)
    return files


def header_declares(header_rel, cpp_class):
    header_path = os.path.join(REPO_ROOT, header_rel)
    if not os.path.isfile(header_path):
        return 'missing'
    try:
        with open(header_path, 'r', encoding='utf-8', errors='replace') as hf:
            content = hf.read()
    except OSError:
        return 'unreadable'
    if re.search(r'\b(class|struct)\s+' + re.escape(cpp_class) + r'\b', content):
        return None
    return 'not_declared'


def main():
    out_of_scope = build_out_of_scope()
    disk_files = find_java_files_on_disk()

    issues = []
    status_counts = {}
    pending = {}

    with open(CSV_PATH, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for i, row in enumerate(reader, start=2):
            java_pkg = (row.get('java_package') or '').strip()
            java_cls = (row.get('java_class') or '').strip()
            java_file = (row.get('java_file') or '').strip()
            cpp_cls = (row.get('cpp_class') or '').strip()
            cpp_header = (row.get('cpp_header') or '').strip()
            status = (row.get('status') or '').strip()
            reason = (row.get('reason') or '').strip()

            valid_statuses = {'ported', 'header_only', 'partial', 'pending', 'out_of_scope'}
            if status not in valid_statuses:
                issues.append(f"Line {i}: unknown status '{status}' for {java_pkg}.{java_cls}")
                continue
            status_counts[status] = status_counts.get(status, 0) + 1

            java_path = java_file
            if java_file != MAZE_RANDOM_MARKER:
                prefix = JAVA_ROOT + '/'
                if java_file.startswith(prefix):
                    java_path = java_file[len(prefix):]
                else:
                    issues.append(f"Line {i}: java_file '{java_file}' outside {JAVA_ROOT}")
                    continue
                if java_path not in disk_files:
                    issues.append(f"Line {i}: java_file on disk? '{java_file}'")
                    continue

            expected_reason = out_of_scope.get(java_path)
            if status == 'out_of_scope':
                if expected_reason is None:
                    issues.append(
                        f"Line {i}: out_of_scope {java_path} not in OUT_OF_SCOPE table (missing reason? misclassified?)")
                elif reason != expected_reason:
                    issues.append(
                        f"Line {i}: out_of_scope {java_path} reason '{reason}' != table '{expected_reason}'")
                elif not reason:
                    issues.append(f"Line {i}: out_of_scope {java_path} has empty reason")
                continue

            if expected_reason is not None:
                issues.append(
                    f"Line {i}: {java_path} is in OUT_OF_SCOPE table ({expected_reason}) but status is '{status}'")

            if status == 'pending':
                pending.setdefault(java_pkg, []).append(java_cls)
                continue

            if status in ('ported', 'header_only'):
                if not cpp_cls:
                    issues.append(f"Line {i}: {status} {java_pkg}.{java_cls} missing cpp_class")
                if not cpp_header:
                    issues.append(f"Line {i}: {status} {java_pkg}.{java_cls} missing cpp_header")
                if cpp_header:
                    err = header_declares(cpp_header, cpp_cls)
                    if err == 'missing':
                        issues.append(f"Line {i}: cpp_header missing {cpp_header} for {java_pkg}.{java_cls}")
                    elif err == 'unreadable':
                        issues.append(f"Line {i}: cpp_header unreadable {cpp_header}")
                    elif err == 'not_declared':
                        issues.append(f"Line {i}: {cpp_header} has no class/struct {cpp_cls}")
                continue

            if status == 'partial':
                if not cpp_header:
                    issues.append(f"Line {i}: partial {java_pkg}.{java_cls} missing cpp_header")
                elif not os.path.isfile(os.path.join(REPO_ROOT, cpp_header)):
                    issues.append(f"Line {i}: cpp_header missing {cpp_header} for partial {java_pkg}.{java_cls}")
                continue

    # Every disk file must appear exactly once in the CSV.
    seen = {}
    with open(CSV_PATH, 'r', encoding='utf-8') as f:
        for row in csv.DictReader(f):
            jf = (row.get('java_file') or '').strip()
            if jf == MAZE_RANDOM_MARKER:
                continue
            prefix = JAVA_ROOT + '/'
            if jf.startswith(prefix):
                p = jf[len(prefix):]
                seen[p] = seen.get(p, 0) + 1
    for p in sorted(disk_files - set(seen)):
        issues.append(f"disk file not in CSV: {p}")
    for p, n in sorted(seen.items()):
        if n > 1:
            issues.append(f"CSV row duplicated for {p}: {n} rows")

    order = ['ported', 'header_only', 'partial', 'pending', 'out_of_scope']
    parts = '  '.join(f'{s}={status_counts.get(s, 0)}' for s in order)
    print(f"class map counts: {parts}")

    if pending:
        print('\npending classes (grouped by package):')
        for pkg in sorted(pending):
            print(f'  {pkg}:')
            for cls in sorted(pending[pkg]):
                print(f'    {cls}')

    missing_reasons = '\n'.join(issues[:50])
    if issues:
        print(f'\nERRORS ({len(issues)}):')
        print(missing_reasons)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())