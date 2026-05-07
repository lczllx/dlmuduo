#!/bin/bash
# Step 2: 预热 Redis — 并发请求热数据，冷数据只留 MySQL
# 默认 92:8 分 (92K hot + 8K cold)，可控参数
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

HOT="${1:-92000}"          # 热数据数量
CODES_FILE="all_codes.txt"
JOBS="${JOBS:-20}"

if [ ! -f "$CODES_FILE" ]; then
    echo "ERROR: $CODES_FILE not found. Run step1_seed_data.py first."
    exit 1
fi

TOTAL=$(wc -l < "$CODES_FILE")
COLD=$((TOTAL - HOT))

echo "Total: $TOTAL | Hot(warm): $HOT | Cold(MySQL-only): $COLD"
echo "Expected hit rate: $((HOT * 100 / TOTAL))%"

# 清空 Redis
redis-cli -h 127.0.0.1 FLUSHDB > /dev/null 2>&1
redis-cli -h 127.0.0.1 CONFIG RESETSTAT > /dev/null 2>&1
echo "Redis reset"

# 随机拆分热/冷
shuf "$CODES_FILE" > _shuffled.txt
head -n "$HOT" _shuffled.txt > warm_codes.txt
tail -n "$COLD" _shuffled.txt > cold_codes.txt
rm _shuffled.txt
echo "warm_codes.txt: $(wc -l < warm_codes.txt) lines"
echo "cold_codes.txt: $(wc -l < cold_codes.txt) lines"

# 并发预热热数据
echo ""
echo "Warming $HOT codes ($JOBS parallel) ..."
t0=$(date +%s)

cat warm_codes.txt | xargs -P "$JOBS" -I {} sh -c '
    curl -s -o /dev/null --max-time 2 "http://localhost:8889/{}"
'

t1=$(date +%s)
elapsed=$((t1 - t0))
echo "Warm done: ${elapsed}s ($((HOT / elapsed)) req/s)"
echo "Redis keys after warm: $(redis-cli -h 127.0.0.1 DBSIZE 2>/dev/null)"

# 批量删除冷数据，确保冷池 codes 一定 miss
if [ "$COLD" -gt 0 ]; then
    echo "Deleting $COLD cold codes from Redis..."
    sed 's/^/DEL /' cold_codes.txt | redis-cli -h 127.0.0.1 --pipe > /dev/null 2>&1
    echo "Redis keys after cold cleanup: $(redis-cli -h 127.0.0.1 DBSIZE 2>/dev/null)"
fi

echo ""
echo "Ready for Step 3: wrk -t4 -c200 -d30s --latency -s step3_wrk_mixed.lua http://localhost:8889/"
