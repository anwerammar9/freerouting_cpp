#!/usr/bin/env bash

set -u

# ============================================================
# Java -> C++ migration loop using OpenCode
# ============================================================

MAX_ITERATIONS=100
PROJECT_DIR="$(pwd)"
LOG_DIR="$PROJECT_DIR/.migration-logs"

mkdir -p "$LOG_DIR"

for ((ITERATION=1; ITERATION<=MAX_ITERATIONS; ITERATION++)); do

    echo
    echo "============================================================"
    echo " Java -> C++ Migration - Iteration $ITERATION"
    echo "============================================================"
    echo

    LOG_FILE="$LOG_DIR/iteration-${ITERATION}.log"

    # --------------------------------------------------------
    # Give the agent the same migration prompt every iteration.
    # The repository state is used to determine what remains.
    # --------------------------------------------------------

    PROMPT=$(cat <<'EOF'
You are working on a Java -> C++ code migration.

Your job is to CONTINUE the migration from the current repository
state. Do not restart the migration and do not redo work that has
already been completed.

Before making changes:

1. Inspect the repository.
2. Read the migration instructions and existing migration state.
3. Inspect the current Java implementation.
4. Inspect the corresponding C++ implementation.
5. Determine exactly what migration work remains.
6. Identify the NEXT concrete piece of work to implement.

Then:

1. Implement the next migration step.
2. Preserve the behavior of the Java implementation.
3. Port or update the corresponding tests.
4. Build the C++ project.
5. Run the relevant tests.
6. Fix compilation errors and test failures caused by your changes.
7. Re-run the tests until they pass.
8. Update the migration tracking/checklist with the work completed.
9. Do not mark anything as complete unless it has been verified.

Important rules:

- Do not blindly convert code.
- Understand the Java implementation before writing C++.
- Preserve public APIs and observable behavior unless the migration
  specification explicitly requires a change.
- Do not remove Java code until the equivalent C++ implementation
  has been verified.
- Do not introduce temporary hacks just to make compilation pass.
- Do not modify unrelated code.
- Do not redo already completed migration steps.
- If you discover additional migration work, record it in the
  migration tracking file.
- Keep the repository buildable whenever possible.

At the end of this iteration:

If the ENTIRE migration is complete and all completion criteria
are satisfied, output exactly:

MIGRATION_COMPLETE

Otherwise output exactly:

MIGRATION_CONTINUE

Do not output MIGRATION_COMPLETE unless you have actually verified
the complete migration.
EOF
)

    # --------------------------------------------------------
    # Run OpenCode
    # --------------------------------------------------------

    echo "$PROMPT" | opencode run 2>&1 | tee "$LOG_FILE"

    EXIT_CODE=${PIPESTATUS[0]}

    echo
    echo "OpenCode exit code: $EXIT_CODE"

    # --------------------------------------------------------
    # Stop if OpenCode says the migration is complete
    # --------------------------------------------------------

    if grep -q "MIGRATION_COMPLETE" "$LOG_FILE"; then
        echo
        echo "============================================================"
        echo " MIGRATION COMPLETE"
        echo "============================================================"
        exit 0
    fi

    # --------------------------------------------------------
    # Stop on OpenCode failure
    # --------------------------------------------------------

    if [ "$EXIT_CODE" -ne 0 ]; then
        echo
        echo "OpenCode failed during iteration $ITERATION."
        echo "Check:"
        echo "  $LOG_FILE"
        exit "$EXIT_CODE"
    fi

    echo
    echo "Iteration $ITERATION completed."
    echo "Migration is not finished. Starting next iteration..."
    echo

    # Small delay so the loop doesn't immediately hammer the CLI.
    sleep 2
done

echo
echo "============================================================"
echo " MAXIMUM ITERATIONS REACHED"
echo "============================================================"
echo
echo "Migration may not be complete."
echo "Check logs in:"
echo "  $LOG_DIR"
echo

exit 1