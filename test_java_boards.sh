#!/bin/bash
# Build and test Java Freerouting on multiple test boards
# Outputs SES files to a specific folder for visualization

set -e

JAVA_HOME="/tmp/jdk-17.0.2"
export PATH="$JAVA_HOME/bin:$PATH"

PROJECT_DIR="/mnt/d/projects/EDA/freerouting_cpp"
OUTPUT_DIR="$PROJECT_DIR/java_output_ses"
JAR="$PROJECT_DIR/build/libs/freerouting.jar"
DEPS="build/libs/gson-2.11.0.jar:build/libs/log4j-api-2.23.1.jar:build/libs/log4j-core-2.23.1.jar"

mkdir -p "$OUTPUT_DIR"

echo "=== Building Java Freerouting ==="
cd "$PROJECT_DIR"
mkdir -p build/classes/java/main
find src -name "*.java" | xargs javac -d build/classes/java/main -cp "build/libs/gson-2.11.0.jar:build/libs/log4j-api-2.23.1.jar:build/libs/log4j-core-2.23.1.jar"
jar cfe build/libs/freerouting.jar app.freerouting.Freerouting -C build/classes/java/main .
echo "Build complete."

echo ""
echo "=== Running on test boards ==="

# List of interesting test boards
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
    if [[ -f "$PROJECT_DIR/tests/data/$dsn" ]]; then
        base=$(basename "$dsn" .dsn)
        out="$OUTPUT_DIR/${base}_java.ses"
        echo "Routing $dsn -> $out"
        timeout 180 java -cp "$JAR:$DEPS" app.freerouting.Freerouting \
            -de "$PROJECT_DIR/tests/data/$dsn" \
            -do "$out" \
            2>&1 | tail -5
        
        if [[ -f "$out" && -s "$out" ]]; then
            echo "  ✓ Success ($(wc -l < "$out") lines)"
        else
            echo "  ✗ Failed or empty output"
        fi
    else
        echo "  Skip: $dsn not found"
    fi
done

echo ""
echo "=== Output files in $OUTPUT_DIR ==="
ls -la "$OUTPUT_DIR"