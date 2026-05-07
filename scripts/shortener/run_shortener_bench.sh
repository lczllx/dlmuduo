#!/bin/bash
# Shortener service benchmark
# Prerequisites: shortener_server running, Redis/MySQL up
# 1. Generate test URLs (skip if short_codes.txt exists and has enough lines)
# 2. Run wrk with keep-alive (default)

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
HOST="${HOST:-http://localhost:8889}"
COUNT="${COUNT:-10000}"
THREADS="${THREADS:-4}"
CONNS="${CONNS:-200}"
DURATION="${DURATION:-30s}"
CODES_FILE="$SCRIPT_DIR/short_codes.txt"

echo "=== Step 1: Generate $COUNT short URLs ==="
if [ -f "$CODES_FILE" ] && [ "$(wc -l < "$CODES_FILE")" -ge "$COUNT" ]; then
    echo "  skip: $CODES_FILE already has $(wc -l < "$CODES_FILE") lines"
else
    cd "$SCRIPT_DIR"
    python3 shortener_gen_urls.py --count "$COUNT" --host "$HOST" --output "$CODES_FILE"
fi

echo ""
echo "=== Step 2: wrk benchmark ($THREADS threads, $CONNS connections, $DURATION) ==="
cd "$SCRIPT_DIR"
wrk -t"$THREADS" -c"$CONNS" -d"$DURATION" --latency -s shortener_wrk.lua "$HOST/"
