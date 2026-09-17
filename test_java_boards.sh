#!/usr/bin/env bash
# Run the Java baseline (Freerouting 2.1.0) headlessly over a set of test boards.
# SES output goes to java_output_ses/ (gitignored).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_DIR="$SCRIPT_DIR/tests/data"
OUTPUT_DIR="${JAVA_OUTPUT_DIR:-$SCRIPT_DIR/java_output_ses}"

# Prefer $FREEROUTING_JAR, otherwise the jar produced by ./gradlew executableJar.
JAR="${FREEROUTING_JAR:-$(ls "$SCRIPT_DIR"/build/libs/*-executable.jar 2>/dev/null | head -1 || true)}"
if [[ -z "${JAR:-}" || ! -f "$JAR" ]]; then
  echo "Executable jar not found. Build it first with './gradlew executableJar'," >&2
  echo "or point FREEROUTING_JAR at an existing freerouting-executable.jar." >&2
  exit 1
fi

mkdir -p "$OUTPUT_DIR"

BOARDS=(
    "empty_board.dsn"
    "BBD_Mars-64.dsn"
    "Issue006-LPC18XX_43XX_SCH.dsn"
    "Issue026-J2_reference.dsn"
    "Issue034-Green14SegLED.dsn"
    "Issue070-Autorouter_FQ101_PCB_2022-05-13.dsn"
    "Issue103-Board-Unrouted.dsn"
    "Issue107-freq_teiler_200kHz_kicad.dsn"
)

for dsn in "${BOARDS[@]}"; do
    src="$DATA_DIR/$dsn"
    if [[ ! -f "$src" ]]; then
        echo "Skip: $dsn not found"
        continue
    fi
    base="$(basename "$dsn" .dsn)"
    out="$OUTPUT_DIR/${base}_java.ses"
    echo "Routing $dsn -> $out"
    timeout 180 java -jar "$JAR" -de "$src" -do "$out" 2>&1 | tail -5 || true
    if [[ -s "$out" ]]; then
        echo "  OK ($(wc -l < "$out") lines)"
    else
        echo "  FAILED or empty output"
    fi
done

echo ""
echo "=== Output files in $OUTPUT_DIR ==="
ls -la "$OUTPUT_DIR"
