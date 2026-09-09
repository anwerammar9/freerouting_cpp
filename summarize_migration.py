import csv

counts = {}
total_pct = 0
count = 0

with open('migration_status.csv') as f:
    reader = csv.DictReader(f)
    for row in reader:
        s = row['Status']
        counts[s] = counts.get(s, 0) + 1
        total_pct += int(row['Percentage'])
        count += 1

print("Status Counts:")
for s, c in counts.items():
    print(f"  {s}: {c}")

if count > 0:
    print(f"\nOverall Migration: {total_pct/count:.2f}%")
else:
    print("\nNo data found.")
