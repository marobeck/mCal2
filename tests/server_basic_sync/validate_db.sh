#!/bin/bash

DB=$1

fail() {
    echo "TEST FAILED: $1"
    exit 1
}

echo "Checking timeblocks..."
TB_COUNT=$(sqlite3 "$DB" "SELECT COUNT(*) FROM timeblocks;")
[[ "$TB_COUNT" == "1" ]] || fail "Expected 1 timeblock"

echo "Checking tasks..."
TASK_COUNT=$(sqlite3 "$DB" "SELECT COUNT(*) FROM tasks;")
[[ "$TASK_COUNT" == "1" ]] || fail "Expected 1 task"

echo "Checking habit entries..."
HABIT_COUNT=$(sqlite3 "$DB" "SELECT COUNT(*) FROM habit_entries;")
[[ "$HABIT_COUNT" == "1" ]] || fail "Expected 1 habit entry"

echo "Checking entry links..."
LINK_COUNT=$(sqlite3 "$DB" "SELECT COUNT(*) FROM entry_links;")
[[ "$LINK_COUNT" == "1" ]] || fail "Expected 1 entry link"

echo "Database validation passed."