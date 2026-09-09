import os

def get_basenames(file_list_path):
    with open(file_list_path, 'r') as f:
        return {os.path.splitext(os.path.basename(line.strip()))[0] for line in f}

java_classes = get_basenames('java_files.txt')
cpp_classes = get_basenames('cpp_files.txt')

missing = sorted(java_classes - cpp_classes)
found = sorted(java_classes & cpp_classes)

print(f"Total Java classes: {len(java_classes)}")
print(f"Total C++ classes: {len(cpp_classes)}")
print(f"Matched classes: {len(found)}")
print(f"Missing classes: {len(missing)}")
print("\nMissing classes list:")
for m in missing:
    print(m)
