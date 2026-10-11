#!/usr/bin/env bash
# Compile tools/java_ref/BoardCounts.java against the executable jar.
# Usage: tools/java_ref/build.sh
# Env: FREEROUTING_JAR (default: /home/anwer/.cache/freerouting-gradle-build/libs/freerouting-2.1.0-executable.jar)
#      JAVA_HOME (default: /home/anwer/.local/jdk-25.0.4.1+1)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JAR="${FREEROUTING_JAR:-/home/anwer/.cache/freerouting-gradle-build/libs/freerouting-2.1.0-executable.jar}"
# The jar is built with JDK 25 (class file version 69); $JAVA_HOME may point at an older JDK
# (e.g. 21, which can only read >= 65), so prefer an explicit JDK 25 unless JAVA25_HOME is set.
JAVAC="${JAVA25_HOME:-/home/anwer/.local/jdk-25.0.4.1+1}/bin/javac"
if [[ ! -x "$JAVAC" ]]; then
  JAVAC="$JAVA_HOME/bin/javac"
fi

if [[ ! -f "$JAR" ]]; then
  echo "Executable jar not found at $JAR. Set FREEROUTING_JAR." >&2
  exit 1
fi

mkdir -p "$SCRIPT_DIR/build"
"$JAVAC" -cp "$JAR" -d "$SCRIPT_DIR/build" "$SCRIPT_DIR/BoardCounts.java"
echo "Compiled BoardCounts -> $SCRIPT_DIR/build/BoardCounts.class"