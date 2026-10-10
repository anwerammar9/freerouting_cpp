#!/usr/bin/env bash
# T007: run the Java reference harness over every tests/data/**/*.dsn and store
# tests/data/expected/<relative-dsn-path>.counts.json (the script keeps going on failures).
# Usage: tools/java_ref/run_all.sh
# Env: FREEROUTING_JAR, JAVA_HOME, TIMEOUT_SEC (default 120 per file)
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
JAR="${FREEROUTING_JAR:-/home/anwer/.cache/freerouting-gradle-build/libs/freerouting-2.1.0-executable.jar}"
JAVA="${JAVA25_HOME:-/home/anwer/.local/jdk-25.0.4.1+1}/bin/java"
if [[ ! -x "$JAVA" ]]; then
  JAVA="$JAVA_HOME/bin/java"
fi
TIMEOUT_SEC="${TIMEOUT_SEC:-120}"
MAX_BYTES=2000000

if [[ ! -f "$SCRIPT_DIR/build/BoardCounts.class" ]]; then
  "$SCRIPT_DIR/build.sh"
fi

OUT_ROOT="$REPO_ROOT/tests/data/expected"
cd "$REPO_ROOT"

fail=0
count=0
while IFS= read -r -d '' dsn; do
  rel="${dsn#tests/data/}"
  out="$OUT_ROOT/$rel.counts.json"
  mkdir -p "$(dirname "$out")"
  count=$((count + 1))

  if ! timeout "$TIMEOUT_SEC" "$JAVA" -Xmx2g -cp "$JAR:$SCRIPT_DIR/build" \
      BoardCounts "$dsn" "$out" > /dev/null 2> "$out.err"; then
    echo "FAILED: $rel" >&2
    fail=$((fail + 1))
    rm -f "$out"
    continue
  fi

  size=$(stat -c %s "$out")
  if [[ "$size" -gt "$MAX_BYTES" ]]; then
    # regenerate without the items array (the oracle still keeps counts/order info elsewhere)
    if timeout "$TIMEOUT_SEC" "$JAVA" -Xmx2g -cp "$JAR:$SCRIPT_DIR/build" \
        BoardCounts "$dsn" "$out" --omit-items > /dev/null 2> "$out.err"; then
      size=$(stat -c %s "$out")
      echo "OMITTED-ITEMS($size B): $rel" >&2
    else
      echo "FAILED(--omit-items): $rel" >&2
      fail=$((fail + 1))
    fi
  fi
  rm -f "$out.err"
  echo "ok ($count): $rel"
done < <(find tests/data -name '*.dsn' -print0 | sort -z)

echo "done: $count files, $fail failures"
exit "$fail"