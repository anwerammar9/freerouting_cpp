import os
import csv

def get_files(directory, extension):
    """Recursively find all files with given extension in directory"""
    file_list = []
    if not os.path.exists(directory):
        return file_list
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(extension):
                rel_path = os.path.relpath(os.path.join(root, file), directory)
                file_list.append(rel_path)
    return file_list

def count_lines(filepath):
    """Count lines in a file"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            return sum(1 for _ in f)
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return 0

# Determine directories to scan
# java_src for C++ project structure, freerouting_java for Java project structure
java_source_dirs = ['freerouting_java', 'java_src/main/java', 'java_src']
cpp_source_dir = 'src'
cpp_include_dir = 'include'

# Find all Java files in the Java source directories
java_files = []
for java_dir in java_source_dirs:
    java_files.extend(get_files(java_dir, '.java'))

# Remove duplicates (keep first found)
seen = set()
unique_java_files = []
for jf in java_files:
    if jf not in seen:
        seen.add(jf)
        unique_java_files.append(jf)
java_files = unique_java_files

# Build maps for C++ files (by basename without extension)
cpp_files_list = get_files(cpp_source_dir, '.cpp')
h_files_list = get_files(cpp_include_dir, '.h')

cpp_map = {}
for f in cpp_files_list:
    basename = os.path.basename(os.path.splitext(f)[0])
    cpp_map[basename] = os.path.join(cpp_source_dir, f)

h_map = {}
for f in h_files_list:
    basename = os.path.basename(os.path.splitext(f)[0])
    h_map[basename] = os.path.join(cpp_include_dir, f)

# Also check if there's a header in the src directory
src_h_files = get_files(cpp_source_dir, '.h')
for f in src_h_files:
    basename = os.path.basename(os.path.splitext(f)[0])
    if basename not in h_map:
        h_map[basename] = os.path.join(cpp_source_dir, f)

print(f"Found {len(java_files)} Java files")
print(f"Found {len(cpp_files_list)} C++ source files")
print(f"Found {len(h_files_list)} C++ header files")

migration_data = []

for jf in sorted(java_files):
    basename = os.path.basename(os.path.splitext(jf)[0])
    
    # Find the actual Java file path
    java_path = None
    for java_dir in java_source_dirs:
        test_path = os.path.join(java_dir, jf)
        if os.path.exists(test_path):
            java_path = test_path
            break
    
    java_lines = count_lines(java_path) if java_path else 0
    
    cpp_path = cpp_map.get(basename)
    h_path = h_map.get(basename)
    
    cpp_lines = count_lines(cpp_path) if cpp_path else 0
    h_lines = count_lines(h_path) if h_path else 0
    
    total_cpp_lines = cpp_lines + h_lines
    
    # Determine status based on line counts
    if cpp_lines > 0 and h_lines > 0:
        status = "Fully Migrated"
        percentage = min(100, int((total_cpp_lines / max(1, java_lines)) * 100)) if java_lines > 0 else 100
    elif cpp_lines > 0 or h_lines > 0:
        status = "Partially Migrated"
        percentage = min(100, int((total_cpp_lines / max(1, java_lines)) * 100)) if java_lines > 0 else 50
    else:
        status = "Not Migrated"
        percentage = 0
    
    migration_data.append({
        'Java File': jf,
        'Class Name': basename,
        'Java Lines': java_lines,
        'C++ Lines': cpp_lines,
        'H Lines': h_lines,
        'Total C++ Lines': total_cpp_lines,
        'Status': status,
        'Percentage': percentage,
        'C++ File': cpp_path if cpp_path else '',
        'H File': h_path if h_path else ''
    })

# Sort by status (Fully Migrated first, then by file name)
status_order = {'Fully Migrated': 0, 'Partially Migrated': 1, 'Not Migrated': 2}
migration_data.sort(key=lambda x: (status_order.get(x['Status'], 3), x['Java File']))

with open('migration_status.csv', 'w', newline='') as csvfile:
    fieldnames = ['Java File', 'Class Name', 'Java Lines', 'C++ Lines', 'H Lines', 'Total C++ Lines', 'Status', 'Percentage', 'C++ File', 'H File']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer.writeheader()
    for data in migration_data:
        writer.writerow(data)

# Print summary
fully_migrated = sum(1 for d in migration_data if d['Status'] == 'Fully Migrated')
partially_migrated = sum(1 for d in migration_data if d['Status'] == 'Partially Migrated')
not_migrated = sum(1 for d in migration_data if d['Status'] == 'Not Migrated')

print(f"\n=== Migration Summary ===")
print(f"Total Java files: {len(migration_data)}")
print(f"Fully Migrated: {fully_migrated}")
print(f"Partially Migrated: {partially_migrated}")
print(f"Not Migrated: {not_migrated}")

if not_migrated > 0:
    print(f"\nNot Migrated files:")
    for d in migration_data:
        if d['Status'] == 'Not Migrated':
            print(f"  - {d['Java File']}")

print(f"\nGenerated migration_status.csv with {len(migration_data)} entries.")