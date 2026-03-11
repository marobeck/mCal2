#!/bin/bash

TEST_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$TEST_ROOT/.." && pwd)"

TMP_DIR="$TEST_ROOT/tmp"
SERVER_PID=""

fail() {
    echo "TEST FAILED: $1"
    exit 1
}

activate_env() {
    if [[ -f "$PROJECT_ROOT/.env/bin/activate" ]]; then
        source "$PROJECT_ROOT/.env/bin/activate"
    else
        fail "Python virtual environment not found at $PROJECT_ROOT/.env"
    fi
}

start_server() {

    mkdir -p "$TMP_DIR"
    rm -f "$TMP_DIR/server.db"

    export MCAL_SERVER_DB="$TMP_DIR/server.db"

    activate_env

    echo "Starting uvicorn server..."

    (
        cd "$PROJECT_ROOT"
        python -m uvicorn server.main:app \
            --host 127.0.0.1 \
            --port 8001 \
            --log-level warning
    ) &

    SERVER_PID=$!

    # Wait for server readiness
    echo "Waiting for server startup..."

    for i in {1..50}; do
        if curl -s http://127.0.0.1:8001/docs >/dev/null 2>&1; then
            echo "Server ready."
            return
        fi
        sleep 0.1
    done

    fail "Server failed to start"
}

stop_server() {
    if [[ -n "${SERVER_PID:-}" ]]; then
        echo "Stopping server..."
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
        SERVER_PID=""
    fi
}

cleanup() {
    stop_server
    rm -rf "$TMP_DIR"
}

trap cleanup EXIT