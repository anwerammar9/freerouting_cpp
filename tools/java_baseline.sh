#!/usr/bin/env bash
# T008: Java baseline runner (replaces test_java_boards.sh).
#
# Reproducible Java reference runs for every board in the E2E set and the benchmark tiers:
# one headless jar run per board with a clean --user_data_path, capturing the SES, the
# result manifest JSON, the stdout+stderr log, the `time -v` block and the wall time.
#
# Usage:
#   tools/java_baseline.sh [--tier smoke|tiny|small|medium|large|stress|e2e|all]
#                          [--mode st|mt] [--timeout S] [board.dsn ...]
#
#   --tier     board set + default per-tier timeout (see TIERS / TIER_TIMEOUT_S below).
#              'e2e' (default) = the 8 boards of test_java_boards.sh.
#   --mode     st = single threaded (`-mt 1`, headline/reference), mt = default threads.
#              default: st
#   --timeout  override the tier timeout in seconds (used for both --router.job_timeout
#              and the outer kill; the outer kill is tier timeout + 60 s).
#
# Environment:
#   FREEROUTING_JAR  path to the executable jar (default: gradle-cache freerouting-2.1.0)
#   JAVA_BIN         java launcher (default: JAVA25_HOME / jdk-25, else JAVA_HOME, else PATH)
#   JAVA_OUTPUT_DIR  output dir (default: <repo>/java_output_ses)
#
# Outputs (java_output_ses/ is gitignored):
#   <board>_java.ses    routed session (wire/via counts feed e2e_compare.sh)
#   <board>_java.json   result manifest (--router.result_json)
#   <board>_java.log    stdout + stderr
#   <board>_java.time   `time -v` block
# and a merged row per board+mode in docs/migration/baseline.json (committed).
#
# Never fails because of the router: crashes/timeouts are recorded as rows with a
# `reason` (timeout / error: <exception> / no-ses / missing-dsn). Exits 0 unless the
# usage is wrong (2) or the jar/java launcher is missing (3).
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DATA_DIR="$ROOT/tests/data"
OUTPUT_DIR="${JAVA_OUTPUT_DIR:-$ROOT/java_output_ses}"
BASELINE_JSON="$ROOT/docs/migration/baseline.json"
STATUS_MD="$ROOT/docs/migration/E2E_STATUS.md"

# ---------------------------------------------------------------- jar + java
JAR="${FREEROUTING_JAR:-}"
if [[ -z "${JAR:-}" ]]; then
  JAR="$(ls "$ROOT"/build/libs/*-executable.jar 2>/dev/null | head -1 || true)"
fi
if [[ -z "${JAR:-}" ]]; then
  JAR="$(ls /home/anwer/.cache/freerouting-gradle-build/libs/*-executable.jar 2>/dev/null | head -1 || true)"
fi
if [[ -z "${JAR:-}" || ! -f "$JAR" ]]; then
  echo "Executable jar not found. Build it with 'gradle executableJar' or set FREEROUTING_JAR." >&2
  exit 3
fi

if [[ -n "${JAVA_BIN:-}" ]]; then
  JAVA="$JAVA_BIN"
else
  JAVA=""
  for cand in "${JAVA25_HOME:-/home/anwer/.local/jdk-25.0.4.1+1}/bin/java" "${JAVA_HOME:-}/bin/java" java; do
    [[ -n "$cand" && -x "$cand" ]] && { JAVA="$cand"; break; }
  done
  [[ -z "$JAVA" ]] && JAVA="java"
fi
if ! command -v "$JAVA" >/dev/null 2>&1 && [[ ! -x "$JAVA" ]]; then
  echo "java launcher not found: $JAVA" >&2
  exit 3
fi

# ---------------------------------------------------------------- board tiers
# Board lists per SOURCE_OF_TRUTH/ROADMAP.md section 9 (bench tiers) and the 8 E2E boards.
TIER_SMOKE=( "tests/data/empty_board.dsn" )
TIER_TINY=(
  "tests/data/Issue026-J2_reference.dsn"
  "tests/data/Issue508-DAC2020_bm01.dsn"
  "tests/data/Issue368-CorneyIslandWireless/corney_island_wireless.dsn"
)
TIER_SMALL=(
  "tests/data/Issue034-Green14SegLED.dsn"
  "tests/data/BBD_Mars-64.dsn"
  "tests/data/Issue107-freq_teiler_200kHz_kicad.dsn"
)
TIER_MEDIUM=(
  "tests/data/Issue029-hw48na.dsn"
  "tests/data/Issue214-freerouting.dsn"
  "tests/data/Issue289-Autorouter_PCB_FHT-8086_2024-03-08.dsn"
)
TIER_LARGE=(
  "tests/data/Issue070-Autorouter_FQ101_PCB_2022-05-13.dsn"
  "tests/data/Issue103-Board-Unrouted.dsn"
  "tests/data/Issue420-contribution-board.dsn"
  "tests/data/Issue555-CNH_Functional_Tester_1.dsn"
)
TIER_STRESS=(
  "tests/data/Issue190-processor.Z80.dsn"
  "tests/data/Issue187-processor.Z80.dsn"
)

# Default per-tier timeout (seconds). Used for --router.job_timeout / --router.optimizer.timeout
# (as HH:MM:SS) and as the base for the outer kill (timeout + 60 s).
declare -A TIER_TIMEOUT_S=(
  [smoke]=60 [tiny]=300 [small]=600 [medium]=900 [large]=1800 [stress]=3600
  [e2e]=1800 [all]=1800
)

# ---------------------------------------------------------------- arg parsing
TIER="e2e"
MODE="st"
TIMEOUT_OVERRIDE=""

usage() {
  echo "usage: $0 [--tier smoke|tiny|small|medium|large|stress|e2e|all] [--mode st|mt] [--timeout S] [board.dsn ...]" >&2
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --tier) TIER="${2:-}"; shift 2 ;;
    --tier=*) TIER="${1#*=}"; shift ;;
    --mode) MODE="${2:-}"; shift 2 ;;
    --mode=*) MODE="${1#*=}"; shift ;;
    --timeout) TIMEOUT_OVERRIDE="${2:-}"; shift 2 ;;
    --timeout=*) TIMEOUT_OVERRIDE="${1#*=}"; shift ;;
    -h|--help) usage; exit 0 ;;
    --) shift; break ;;
    -*) echo "unknown option: $1" >&2; usage; exit 2 ;;
    *) break ;;
  esac
done

if [[ "$MODE" != "st" && "$MODE" != "mt" ]]; then
  echo "--mode must be st or mt (got '$MODE')" >&2
  exit 2
fi
if [[ -n "$TIMEOUT_OVERRIDE" && ! "$TIMEOUT_OVERRIDE" =~ ^[0-9]+$ ]]; then
  echo "--timeout must be an integer number of seconds (got '$TIMEOUT_OVERRIDE')" >&2
  exit 2
fi

# e2e board list is taken verbatim from test_java_boards.sh (single source of truth).
e2e_boards() {
  sed -n '/^BOARDS=(/,/^)/p' "$ROOT/test_java_boards.sh" | grep -o '"[^"]*\.dsn"' | tr -d '"'
}

# ---------------------------------------------------------------- board list
BOARDS=()
if [[ $# -gt 0 ]]; then
  BOARDS=("$@")
else
  case "$TIER" in
    smoke) BOARDS=("${TIER_SMOKE[@]}") ;;
    tiny)  BOARDS=("${TIER_TINY[@]}") ;;
    small) BOARDS=("${TIER_SMALL[@]}") ;;
    medium) BOARDS=("${TIER_MEDIUM[@]}") ;;
    large) BOARDS=("${TIER_LARGE[@]}") ;;
    stress) BOARDS=("${TIER_STRESS[@]}") ;;
    e2e)   while IFS= read -r b; do BOARDS+=("tests/data/$b"); done < <(e2e_boards) ;;
    all)
      while IFS= read -r b; do BOARDS+=("tests/data/$b"); done < <(e2e_boards)
      BOARDS+=("${TIER_TINY[@]}" "${TIER_SMALL[@]}" "${TIER_MEDIUM[@]}"
                "${TIER_LARGE[@]}" "${TIER_STRESS[@]}")
      ;;
    *) echo "unknown tier: $TIER" >&2; usage; exit 2 ;;
  esac
fi
if [[ ${#BOARDS[@]} -eq 0 ]]; then
  echo "no boards selected" >&2
  exit 2
fi

# ---------------------------------------------------------------- timeouts
TIER_S="${TIER_TIMEOUT_S[$TIER]:-600}"
if [[ -n "$TIMEOUT_OVERRIDE" ]]; then
  TIER_S="$TIMEOUT_OVERRIDE"
fi
OUTER_S=$((TIER_S + 60))

hhmmss() {
  local s="$1"
  printf '%02d:%02d:%02d' $(( s / 3600 )) $(( (s % 3600) / 60 )) $(( s % 60 ))
}
JOB_TIMEOUT="$(hhmmss "$TIER_S")"

mkdir -p "$OUTPUT_DIR"

# ---------------------------------------------------------------- helpers
# Counts '(wire' / '(via' tokens inside (network_out ...) — same as tools/e2e_compare.sh.
count_in_network_out() {
  local file="$1" token="$2"
  if [[ ! -s "$file" ]]; then
    echo "0"
    return
  fi
  awk -v tok="$token" '
    /\(network_out/ { inside = 1 }
    inside && $0 ~ "^[ \t]*\\(" tok "([ \t]|$)" { n++ }
    END { print n + 0 }
  ' "$file"
}

# Best-effort reason why no SES was produced.
#   timeout            -> outer kill (exit 124/137)
#   error: <Exception> -> first exception class in the log
#   parse error        -> reader could not parse the DSN
#   no-ses             -> exited 0 but wrote nothing
classify_reason() {
  local exit_code="$1" log="$2" ses="$3" out_json="$4"
  if [[ -s "$ses" ]]; then
    echo ""
    return
  fi
  if [[ "$exit_code" -eq 124 || "$exit_code" -eq 137 ]]; then
    echo "timeout"
    return
  fi
  # First exception / error class from the log (the throw site is at the top of the stack).
  local exc
  exc="$(grep -Eo '^[a-zA-Z0-9_.$]+(Exception|Error)(:[^[]*)?' "$log" 2>/dev/null | head -1 || true)"
  if [[ -z "$exc" ]]; then
    exc="$(grep -Eo '^[a-zA-Z0-9_.$]+\.(Exception|Error)' "$log" 2>/dev/null | head -1 || true)"
  fi
  if [[ -n "$exc" ]]; then
    if [[ "$exc" == *"Exception"* || "$exc" == *"Error"* ]]; then
      # The DSN reader failing to parse the file surfaces as board == null -> NPE.
      if grep -q 'Parse error\|parse error\|Could not parse\|Unexpected token' "$log" 2>/dev/null; then
        echo "parse error"
      else
        echo "error: ${exc%%:*}"
      fi
      return
    fi
  fi
  if [[ -s "$out_json" ]] && ! grep -q '"error"' "$out_json" 2>/dev/null; then
    :
  fi
  echo "no-ses"
}

# Resolve a board argument (path or bare name) to an existing DSN path, or "".
resolve_dsn() {
  local b="$1"
  if [[ -f "$b" ]]; then
    echo "$b"
    return
  fi
  for cand in "$DATA_DIR/$b" "$DATA_DIR/${b%.dsn}.dsn" "$DATA_DIR/$b/${b}.dsn"; do
    if [[ -f "$cand" ]]; then
      echo "$cand"
      return
    fi
  done
  # last resort: any *.dsn under tests/data whose basename matches
  local found
  found="$(find "$DATA_DIR" -name "${b%.dsn}.dsn" -print -quit 2>/dev/null)"
  echo "$found"
}

JAR_SHA="$(sha256sum "$JAR" | cut -d' ' -f1)"
JDK_VER="$("$JAVA" -version 2>&1 | sed -n 's/.*version "\([^"]*\)".*/\1/p' | head -1)"
[[ -z "$JDK_VER" ]] && JDK_VER="$("$JAVA" -version 2>&1 | head -1)"
RUN_DATE="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
TIME_BIN="${TIME_BIN:-/usr/bin/time}"
[[ -x "$TIME_BIN" ]] || TIME_BIN=""

TMP_RECORDS="$(mktemp)"
trap 'rm -f "$TMP_RECORDS"' EXIT

echo "java_baseline: tier=$TIER mode=$MODE job_timeout=$JOB_TIMEOUT outer=${OUTER_S}s"
echo "  jar=$JAR"
echo "  java=$JAVA (jdk $JDK_VER)"
echo "  output=$OUTPUT_DIR"
echo

# ---------------------------------------------------------------- run loop
# Sequential on purpose: the box has ~1.3 GB free RAM.
for entry in "${BOARDS[@]}"; do
  dsn="$(resolve_dsn "$entry")"
  base="$(basename "${entry%.dsn}" .dsn)"

  if [[ -z "$dsn" ]]; then
    echo "MISSING dsn: $entry"
    printf '{"board":"%s","mode":"%s","exit":null,"wall_s":null,"user_s":null,"sys_s":null,"max_rss_kb":null,"timed_out":false,"ses_bytes":0,"wires":0,"vias":0,"jdk":"%s","jar_sha256":"%s","date":"%s","reason":"missing-dsn"}\n' \
      "$base" "$MODE" "$JDK_VER" "$JAR_SHA" "$RUN_DATE" >>"$TMP_RECORDS"
    continue
  fi

  out_base="$OUTPUT_DIR/${base}_java"
  ses="$out_base.ses"
  out_json="$out_base.json"
  log="$out_base.log"
  time_file="$out_base.time"
  rm -f "$ses" "$out_json" "$time_file"

  user_data="$(mktemp -d)"

  echo "Routing $base (timeout ${JOB_TIMEOUT} / outer ${OUTER_S}s)..."

  cmd=("$JAVA" -Xmx2g -jar "$JAR" -de "$dsn" -do "$ses" -mp 100 -dl -ll INFO
       "--user_data_path=$user_data" "--router.result_json=$out_json"
       "--router.job_timeout=$JOB_TIMEOUT" "--router.optimizer.timeout=$JOB_TIMEOUT")
  [[ "$MODE" == "st" ]] && cmd+=(-mt 1)

  start_ns="$(date +%s%N)"
  if [[ -n "$TIME_BIN" ]]; then
    "$TIME_BIN" -o "$time_file" -v timeout --kill-after=10 "$OUTER_S" "${cmd[@]}" >"$log" 2>&1
  else
    timeout --kill-after=10 "$OUTER_S" "${cmd[@]}" >"$log" 2>&1
  fi
  exit_code=$?
  end_ns="$(date +%s%N)"

  wall_s="$(awk -v a="$start_ns" -v b="$end_ns" 'BEGIN { printf "%.1f", (b - a) / 1e9 }')"

  # `time -v` fields (empty when /usr/bin/time is unavailable).
  user_s="$(sed -n 's/^[ \t]*User time (seconds): *//p' "$time_file" 2>/dev/null | head -1)"
  sys_s="$(sed -n 's/^[ \t]*System time (seconds): *//p' "$time_file" 2>/dev/null | head -1)"
  max_rss="$(sed -n 's/^[ \t]*Maximum resident set size (kbytes): *//p' "$time_file" 2>/dev/null | head -1)"

  timed_out=false
  [[ $exit_code -eq 124 || $exit_code -eq 137 ]] && timed_out=true
  # Java finishes with state COMPLETED even when the job/optimizer timeout cut the run short;
  # such runs are wall-clock limited (not reproducible) — flag them.
  hit_job_timeout="$(awk -v w="$wall_s" -v t="$TIER_S" 'BEGIN { print (w + 0 >= t - 5) ? "true" : "false" }')"

  reason="$(classify_reason "$exit_code" "$log" "$ses" "$out_json")"
  ses_bytes=0
  [[ -s "$ses" ]] && ses_bytes="$(stat -c %s "$ses")"
  wires="$(count_in_network_out "$ses" wire)"
  vias="$(count_in_network_out "$ses" via)"

  # Keep the pre-existing session readable by humans when nothing was routed.
  if [[ ! -s "$ses" && ! -e "$ses" ]]; then
    : # no session at all — the reason is recorded in the summary
  fi

  status="ok"
  [[ -n "$reason" ]] && status="$reason"

  echo "  exit=$exit_code wall=${wall_s}s ses=${ses_bytes}B wires=$wires vias=$vias ${status:+($status)}"

  printf '{"board":"%s","mode":"%s","exit":%d,"wall_s":%s,"user_s":%s,"sys_s":%s,"max_rss_kb":%s,"timed_out":%s,"hit_job_timeout":%s,"ses_bytes":%d,"wires":%d,"vias":%d,"jdk":"%s","jar_sha256":"%s","date":"%s","reason":"%s"}\n' \
    "$base" "$MODE" "$exit_code" \
    "${wall_s:-null}" "${user_s:-null}" "${sys_s:-null}" "${max_rss:-null}" \
    "$timed_out" "$hit_job_timeout" "$ses_bytes" "$wires" "$vias" "$JDK_VER" "$JAR_SHA" "$RUN_DATE" "$reason" \
    >>"$TMP_RECORDS"

  rm -rf "$user_data"
done

# ---------------------------------------------------------------- summary + docs
echo
python3 - "$BASELINE_JSON" "$TMP_RECORDS" "$STATUS_MD" "$TIER" "$MODE" "$JOB_TIMEOUT" <<'PY'
import json, os, sys, datetime

baseline_path, records_path, status_md, tier, mode, job_timeout = sys.argv[1:7]

new_rows = []
with open(records_path) as fh:
    for line in fh:
        line = line.strip()
        if line:
            new_rows.append(json.loads(line))

rows = []
if os.path.exists(baseline_path):
    try:
        with open(baseline_path) as fh:
            rows = json.load(fh)
        if not isinstance(rows, list):
            rows = []
    except (json.JSONDecodeError, OSError):
        rows = []

# Merge by (board, mode): the newest run wins, older unrelated runs are kept.
key = lambda r: (r.get("board"), r.get("mode"))
merged = {key(r): r for r in rows if r.get("board")}
for r in new_rows:
    merged[key(r)] = r
out = [merged[k] for k in sorted(merged, key=lambda k: (k[0] or "", k[1] or ""))]

with open(baseline_path, "w") as fh:
    json.dump(out, fh, indent=2)
    fh.write("\n")

# ---- refresh the "Java baselines" section of docs/migration/E2E_STATUS.md ----
BEGIN = "<!-- java-baseline:begin -->"
END = "<!-- java-baseline:end -->"

lines = [
    "",
    BEGIN,
    "",
    "## Java baselines",
    "",
    "Last run: `tools/java_baseline.sh --tier %s --mode %s` (job timeout %s, outer kill "
    "+60 s); rows from earlier runs are kept (each row's date is in baseline.json), merged into `docs/migration/baseline.json`. `reason` is empty on success; "
    "otherwise `timeout` (outer kill), `error: <exception>`, `parse error` (the DSN is not a "
    "parseable Specctra file) or `missing-dsn`." % (tier, mode, job_timeout),
    "",
    "Why 4 of the 8 E2E boards had no Java baseline before T008 (old `test_java_boards.sh`, "
    "180 s outer timeout): BBD_Mars-64 (170 s, at the edge of the limit), Issue070 (318 s) and Issue103-Board-Unrouted "
    "(1806 s, ends at the job limit) need about 180 s or more in single-thread mode; "
    "Issue006-LPC18XX_43XX_SCH is a binary OLE file, so Java fails with "
    "`Not a Specctra DSN file: expected '(pcb <name>' header` (the correct result).",
    "",
    "`Job limit` = the run used the whole job timeout (Java still reports COMPLETED, but the "
    "result is wall-clock limited and not reproducible).",
    "",
    "| Board | Mode | Exit | Wall (s) | Timed out | Job limit | SES (B) | Wires | Vias | Reason |",
    "|-------|------|------|----------|-----------|-----------|---------|-------|------|--------|",
]
for r in out:
    reason = r.get("reason") or ""
    if reason:
        reason = "`%s`" % reason
    wall = r.get("wall_s")
    wall = "" if wall is None else ("%.1f" % wall)
    lines.append("| %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |" % (
        r.get("board", "?"), r.get("mode", "?"),
        "" if r.get("exit") is None else r.get("exit"),
        wall,
        "yes" if r.get("timed_out") else "no",
        "yes" if r.get("hit_job_timeout") else "no",
        r.get("ses_bytes", 0), r.get("wires", 0), r.get("vias", 0),
        reason,
    ))
lines += [END, ""]

section = "\n".join(lines)

if os.path.exists(status_md):
    with open(status_md) as fh:
        content = fh.read()
else:
    content = ""

if BEGIN in content and END in content:
    head = content.split(BEGIN)[0].rstrip("\n")
    tail = content.split(END, 1)[1]
    content = head + "\n" + section + tail.lstrip("\n")
else:
    content = content.rstrip("\n") + "\n" + section

with open(status_md, "w") as fh:
    fh.write(content)

ok = sum(1 for r in out if not r.get("reason"))
missing = [r for r in out if r.get("reason")]
print("baseline.json: %d rows (%d routed ok, %d with a reason)" % (len(out), ok, len(missing)))
for r in missing:
    print("  %-45s %s" % (r.get("board"), r.get("reason")))
print("updated %s" % status_md)
PY

echo
echo "outputs in $OUTPUT_DIR"
exit 0
