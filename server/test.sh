#!/bin/bash

set -e

echo "Starting mCal2 sync server..."

# Start server in background
uvicorn main:app --host 127.0.0.1 --port 8000 &
SERVER_PID=$!

# Give server time to start
sleep 2

echo ""
echo "Sending test sync request..."

RESPONSE=$(curl -s -X POST http://127.0.0.1:8000/sync \
  -H "Content-Type: application/json" \
  -d '{
    "client_id": "test-client",
    "last_server_version": 0,
    "entries": [

      {
        "table": "timeblocks",
        "data": {
          "uuid": "tb1",
          "status": 0,
          "name": "Morning Block",
          "description": "Start of day",
          "day_frequency": 127,
          "duration": 3600,
          "start": 800,
          "day_start": 0,
          "completed_datetime": null,
          "modified_at": 1741023001,
          "deleted_at": null
        }
      },

      {
        "table": "tasks",
        "data": {
          "uuid": "task1",
          "timeblock_uuid": "tb1",
          "name": "Test Task",
          "description": "Testing sync",
          "due_date": null,
          "priority": 1,
          "scope": 1,
          "status": 0,
          "goal_spec": 0,
          "completed_datetime": null,
          "modified_at": 1741023002,
          "deleted_at": null
        }
      },

      {
        "table": "habit_entries",
        "data": {
          "task_uuid": "task1",
          "date": "2026-03-02",
          "modified_at": 1741023003,
          "deleted_at": null
        }
      },

      {
        "table": "entry_links",
        "data": {
          "parent_uuid": "task1",
          "child_uuid": "task1",
          "link_type": 1,
          "modified_at": 1741023004,
          "deleted_at": null
        }
      }

    ]
  }')

echo ""
echo "Server Response:"
echo "$RESPONSE" | jq .

echo ""
echo "Verifying database state..."
echo ""

echo "---- Global Version ----"
sqlite3 data/server.db "SELECT * FROM server_state;"

echo ""
echo "---- timeblocks ----"
sqlite3 data/server.db "SELECT * FROM timeblocks;"

echo ""
echo "---- tasks ----"
sqlite3 data/server.db "SELECT * FROM tasks;"

echo ""
echo "---- habit_entries ----"
sqlite3 data/server.db "SELECT * FROM habit_entries;"

echo ""
echo "---- entry_links ----"
sqlite3 data/server.db "SELECT * FROM entry_links;"

echo ""
echo "---- Ledgers ----"
sqlite3 data/server.db "SELECT * FROM timeblocks_ledger;"
sqlite3 data/server.db "SELECT * FROM tasks_ledger;"
sqlite3 data/server.db "SELECT * FROM habit_entries_ledger;"
sqlite3 data/server.db "SELECT * FROM entry_links_ledger;"

echo ""
echo "Stopping server..."
kill $SERVER_PID

echo "Test complete."