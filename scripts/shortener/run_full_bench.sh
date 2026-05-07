#!/bin/bash
# 完整三步压测：造数据 → 预热 Redis → wrk
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

HOST="${HOST:-http://localhost:8889}"
TOTAL="${TOTAL:-100000}"
WARM="${WARM:-80000}"
THREADS="${THREADS:-4}"
CONNS="${CONNS:-200}"
DUR="${DUR:-30s}"

echo "============================================"
echo "  Step 1/3: Seed $TOTAL rows → MySQL only"
echo "============================================"
python3 step1_seed_data.py "$TOTAL" | docker exec -i mysql-short mysql -uroot -pshortener shortener 2>/dev/null

echo ""
echo "============================================"
echo "  Step 2/3: Warm up $WARM codes → Redis"
echo "============================================"
bash step2_warm_redis.sh "$WARM"

echo ""
echo "============================================"
echo "  Step 3/3: wrk benchmark"
echo "  ($TOTAL codes, ~80% Redis hit, ~20% MySQL fallback)"
echo "============================================"
wrk -t"$THREADS" -c"$CONNS" -d"$DUR" --latency \
    -s step3_wrk_mixed.lua "$HOST/"

echo ""
echo "=== Redis hit rate ==="
redis-cli -h 127.0.0.1 INFO stats | grep keyspace
