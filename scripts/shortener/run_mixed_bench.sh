#!/bin/bash
# 真实分布压测：80% Redis 命中 + 15% MySQL 回源 + 5% 404
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
HOST="${HOST:-http://localhost:8889}"
THREADS="${THREADS:-4}"
CONNS="${CONNS:-200}"
DUR="${DUR:-30s}"

cd "$SCRIPT_DIR"

# 确保有测试数据
if [ ! -f short_codes.txt ] || [ "$(wc -l < short_codes.txt)" -lt 1000 ]; then
    echo "=== Generating test data ==="
    python3 shortener_gen_urls.py --count 1000 --host "$HOST" --output short_codes.txt
fi

echo "=== wrk $THREADS threads, $CONNS connections, $DUR ==="
wrk -t"$THREADS" -c"$CONNS" -d"$DUR" --latency -s shortener_mixed_wrk.lua "$HOST/"
