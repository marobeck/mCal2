#!/bin/bash
# This test starts a server with an isolated database, sends a sync request with a predefined payload

set -euo pipefail

# Get the directory of this test script
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

source "$(dirname "$0")/../utils.sh"

start_server

echo "Sending sync payload..."

curl -s -X POST http://127.0.0.1:8001/sync \
  -H "Content-Type: application/json" \
  -d @"$TEST_DIR/payload.json" \
  > "$TMP_DIR/response.json"

echo "Validating server response..."
jq . "$TMP_DIR/response.json"

echo "Validating database state..."
echo "$TMP_DIR/server.db"

bash "$TEST_DIR/validate_db.sh" "$TMP_DIR/server.db"

echo "Test passed."