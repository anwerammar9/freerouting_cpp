#!/usr/bin/env python3
import csv
import os
import re
import sys

REPO_ROOT = '/home/anwer/projects/freerouting_cpp'
OUT_OF_SCOPE_KEYWORDS = [
    'gui', 'api', 'analytics', 'io.kicad', 'util.gson', 'settings.sources',
    'io.export', 'io.intermediate', 'io.kicad', 'io.logging', 'io.serialize',
    'util.file', 'util.registry', 'util.xml', 'util.json',
]

def is_out_of_scope(java_pkg, java_cls):
    pkg_lower = (java_pkg or '').lower()
    cls_lower = (java_cls or '').strip().lower()
    # Check full package path
    full = f"{pkg_lower}.{cls_lower}" if pkg_lower else cls_lower
    for k in OUT_OF_SCOPE_KEYWORDS:
        if k in pkg_lower or k in cls_lower or k in full:
            return True
    if pkg_lower.startswith('gui') or pkg_lower.startswith('api') or 'io.kicad' in pkg_lower:
        return True
    # settings.sources related
    if 'sources' in pkg_lower:
        return True
    # Many UI/action classes that aren't ported - common patterns
    ui_indicators = ['actions', 'dialogs', 'windows', 'gui', 'graphics', 'palette']
    if any(ind in pkg_lower for ind in ui_indicators):
        # But some actions are core logic - be selective
        if 'actions' in pkg_lower and cls_lower in ['drillitemmover', 'forcedpadrouter', 'forcedvia', 
                                                     'itemidgenerator', 'iteminfoprinter', 'packageselector',
                                                     'selectionaction', 'structureupdate', 'traceclean']:
            pass  # might be core
        elif 'optimize' in pkg_lower:
            pass  # optimization logic - need to check
        else:
            # many others are UI
            if 'optimize' not in pkg_lower:
                return True
    return False

def find_class_in_cpp(class_name):
    # Search for class/struct declaration
    for root, dirs, files in os.walk(os.path.join(REPO_ROOT, 'src')):
        # skip build
        dirs[:] = [d for d in dirs if d != 'build']
        for f in files:
            if f.endswith('.h'):
                path = os.path.join(root, f)
                try:
                    with open(path, 'r', encoding='utf-8', errors='replace') as hf:
                        content = hf.read()
                        if re.search(r'\b(class|struct)\s+' + re.escape(class_name) + r'\b', content):
                            return os.path.relpath(path, REPO_ROOT)
                except:
                    pass
    return None

def main():
    csv_path = os.path.join(REPO_ROOT, 'JavaToCppClassMap.csv')
    issues = []
    out_of_scope = 0
    ok = 0
    fixed_suggestions = []  # for manual fix
    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for i, row in enumerate(reader, start=2):
            java_pkg = (row.get('java_package') or '').strip()
            java_cls = (row.get('java_class') or '').strip()
            cpp_cls = (row.get('cpp_class') or '').strip()
            cpp_header = (row.get('cpp_header') or '').strip()
            status = (row.get('status') or '').strip().lower()
            
            if is_out_of_scope(java_pkg, java_cls) and status != 'ported':
                out_of_scope += 1
                continue
            
            if status == 'out_of_scope':
                out_of_scope += 1
                continue
            
            if not cpp_cls or not cpp_header:
                # Try to find it
                found = find_class_in_cpp(java_cls)
                if found:
                    fixed_suggestions.append(f"Line {i}: suggest {java_cls} -> {java_cls} in {found}")
                issues.append(f"Line {i}: missing cpp for {java_pkg}.{java_cls}")
                continue
            
            header_path = os.path.join(REPO_ROOT, cpp_header)
            if not os.path.exists(header_path):
                # Try alternative
                found = find_class_in_cpp(cpp_cls)
                if found:
                    fixed_suggestions.append(f"Line {i}: header wrong for {cpp_cls}: {cpp_header} vs {found}")
                issues.append(f"Line {i}: header missing {cpp_header}")
                continue
            
            try:
                with open(header_path, 'r', encoding='utf-8', errors='replace') as hf:
                    content = hf.read()
            except:
                issues.append(f"Line {i}: read error {cpp_header}")
                continue
            
            if not re.search(r'\b(class|struct)\s+' + re.escape(cpp_cls) + r'\b', content):
                # maybe name differs?
                found = find_class_in_cpp(cpp_cls)
                if found is None:
                    # look for similar
                    pass
                issues.append(f"Line {i}: {cpp_header} no decl for {cpp_cls}")
                continue
            ok += 1
    
    print(f"ok={ok} out_of_scope={out_of_scope} issues={len(issues)}")
    if fixed_suggestions:
        print('\n--- SUGGESTIONS ---')
        for s in fixed_suggestions[:20]:
            print(s)
    if issues:
        for issue in issues[:30]:
            print(issue)
        return 1
    return 0

if __name__ == '__main__':
    sys.exit(main())
