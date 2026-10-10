#!/usr/bin/env python3
"""Tiny unit test for tools/class_map_check.py (stdlib unittest, not part of ctest).

Builds a throwaway repo tree with one Java class per status, writes a matching
JavaToCppClassMap.csv, and asserts the checker accepts it and prints counts.

Run with: python3 tools/test_class_map_check.py
"""
import csv
import os
import subprocess
import sys
import tempfile
import unittest

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
CHECKER = os.path.join(TOOLS_DIR, 'class_map_check.py')


def run_checker(repo_root, csv_path):
    env = dict(os.environ)
    env['CLASS_MAP_ROOT'] = repo_root
    env['CLASS_MAP_CSV'] = csv_path
    return subprocess.run(
        [sys.executable, CHECKER],
        capture_output=True,
        text=True,
        env=env,
    )


class ClassMapCheckTest(unittest.TestCase):

    def _build_tree(self):
        tmp = tempfile.TemporaryDirectory()
        java_root = os.path.join(tmp.name, 'java/app/freerouting')
        os.makedirs(java_root)
        os.makedirs(os.path.join(tmp.name, 'src/alpha'))
        os.makedirs(os.path.join(java_root, 'alpha'))
        os.makedirs(os.path.join(java_root, 'gui'))

        with open(os.path.join(java_root, 'alpha', 'Ported.java'), 'w') as f:
            f.write('package alpha; class Ported {}\n')
        with open(os.path.join(java_root, 'alpha', 'HeaderOnly.java'), 'w') as f:
            f.write('package alpha; class HeaderOnly {}\n')
        with open(os.path.join(java_root, 'alpha', 'Partial.java'), 'w') as f:
            f.write('package alpha; class Partial {}\n')
        with open(os.path.join(java_root, 'alpha', 'Pending.java'), 'w') as f:
            f.write('package alpha; class Pending {}\n')
        with open(os.path.join(java_root, 'gui', 'Widget.java'), 'w') as f:
            f.write('package gui; class Widget {}\n')

        with open(os.path.join(tmp.name, 'src/alpha', 'Ported.h'), 'w') as f:
            f.write('#pragma once\nnamespace alpha { class Ported {}; }\n')
        with open(os.path.join(tmp.name, 'src/alpha', 'HeaderOnly.h'), 'w') as f:
            f.write('#pragma once\nnamespace alpha { struct HeaderOnly {}; }\n')
        with open(os.path.join(tmp.name, 'src/alpha', 'Partial.h'), 'w') as f:
            f.write('#pragma once\nnamespace alpha { class PartialOwner {}; }\n')

        csv_path = os.path.join(tmp.name, 'JavaToCppClassMap.csv')
        with open(csv_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['java_package', 'java_class', 'java_file',
                             'cpp_class', 'cpp_header', 'status', 'reason'])
            writer.writerow(['alpha', 'Ported', 'java/app/freerouting/alpha/Ported.java',
                             'Ported', 'src/alpha/Ported.h', 'ported', ''])
            writer.writerow(['alpha', 'HeaderOnly', 'java/app/freerouting/alpha/HeaderOnly.java',
                             'HeaderOnly', 'src/alpha/HeaderOnly.h', 'header_only', ''])
            writer.writerow(['alpha', 'Partial', 'java/app/freerouting/alpha/Partial.java',
                             'PartialOwner', 'src/alpha/Partial.h', 'partial', ''])
            writer.writerow(['alpha', 'Pending', 'java/app/freerouting/alpha/Pending.java',
                             '', '', 'pending', ''])
            writer.writerow(['gui', 'Widget', 'java/app/freerouting/gui/Widget.java',
                             '', '', 'out_of_scope', 'gui'])
        return tmp

    def test_accepts_valid_map(self):
        with self._build_tree() as tmp:
            res = run_checker(tmp, os.path.join(tmp, 'JavaToCppClassMap.csv'))
            self.assertEqual(res.returncode, 0, res.stdout + res.stderr)
            self.assertIn('ported=1', res.stdout)
            self.assertIn('header_only=1', res.stdout)
            self.assertIn('partial=1', res.stdout)
            self.assertIn('pending=1', res.stdout)
            self.assertIn('out_of_scope=1', res.stdout)


if __name__ == '__main__':
    unittest.main()