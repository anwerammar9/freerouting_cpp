#!/usr/bin/env bash
# Route one DSN board with the C++ freerouting binary and compare the SES against the Java baseline.
#
# Usage: tools/e2e_compare.sh [--header] <board.dsn>
#   E2E_TIMEOUT   seconds before the C++ run is killed (default 300)
#   E2E_OUT_DIR   where the C++ SES and log go (default build/e2e)
#   FREEROUTING   C++ binary (default build/freerouting)
#
# Prints exactly one '|'-separated line:
#   board | status | exit | seconds | cpp_wires | java_wires | cpp_vias | java_vias
# status is ok / timeout / crash / error / no-ses. Never fails because of the router: crashes and
# timeouts are reported in the row and the script exits 0 (it exits 2 only on bad usage).
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TIMEOUT_S="${E2E_TIMEOUT:-300}"
OUT_DIR="${E2E_OUT_DIR:-$ROOT/build/e2e}"
BIN="${FREEROUTING:-$ROOT/build/freerouting}"
JAVA_DIR="${JAVA_SES_DIR:-$ROOT/java_output_ses}"

if [[ "${1:-}" == "--header" ]]; then
  echo "board | status | exit | seconds | cpp_wires | java_wires | cpp_vias | java_vias"
  shift
  [[ $# -eq 0 ]] && exit 0
fi
if [[ $# -ne 1 ]]; then
  echo "usage: $0 [--header] <board.dsn>" >&2
  exit 2
fi

DSN="$1"
BASE="$(basename "$DSN" .dsn)"
mkdir -p "$OUT_DIR"
SES="$OUT_DIR/${BASE}_cpp.ses"
LOG="$OUT_DIR/${BASE}_cpp.log"
JAVA_SES="$JAVA_DIR/${BASE}_java.ses"
rm -f "$SES"

# Counts '(wire' / '(via' tokens inside (network_out ...). Prints "n/a" for a missing/empty file.
count_in_network_out() {
  local file="$1" token="$2"
  if [[ ! -s "$file" ]]; then
    echo "n/a"
    return
  fi
  awk -v tok="$token" '
    /\(network_out/ { inside = 1 }
    inside && $0 ~ "^[ \t]*\\(" tok "([ \t]|$)" { n++ }
    END { print n + 0 }
  ' "$file"
}

status="ok"
code=0
seconds=0
if [[ ! -f "$DSN" ]]; then
  status="error"
  code="-"
elif [[ ! -x "$BIN" ]]; then
  status="error"
  code="no-binary"
else
  start=$(date +%s.%N)
  timeout --kill-after=10 "$TIMEOUT_S" "$BIN" -de "$DSN" -do "$SES" >"$LOG" 2>&1
  code=$?
  end=$(date +%s.%N)
  seconds=$(awk -v a="$start" -v b="$end" 'BEGIN { printf "%.1f", b - a }')
  if [[ $code -eq 124 || $code -eq 137 ]]; then
    status="timeout"
  elif [[ $code -ge 128 ]]; then
    status="crash"
  elif [[ $code -ne 0 ]]; then
    status="error"
  elif [[ ! -s "$SES" ]]; then
    status="no-ses"
  fi
fi

echo "$BASE | $status | $code | $seconds" \
  "| $(count_in_network_out "$SES" wire) | $(count_in_network_out "$JAVA_SES" wire)" \
  "| $(count_in_network_out "$SES" via) | $(count_in_network_out "$JAVA_SES" via)"
exit 0
