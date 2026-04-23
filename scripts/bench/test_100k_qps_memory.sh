#!/bin/bash
# 测试十万并发连接下的 QPS 和内存占用
# 路径相对于仓库根: scripts/bench/test_100k_qps_memory.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$REPO_ROOT/bin"

HOST=127.0.0.1
PORT=8889
WRK_THREADS=4
WRK_CONNECTIONS=10000
WRK_DURATION=30s

# 检查服务器是否运行
if ! ss -tlnp 2>/dev/null | grep -q ":$PORT "; then
    echo "请先启动 HTTP 服务: $BIN/http_server（十万并发可用 $SCRIPT_DIR/run_http_server_for_100k.sh）"
    exit 1
fi

SERVER_PID=$(ss -tlnp 2>/dev/null | grep ":$PORT " | grep -oP 'pid=\K[0-9]+' | head -1)
if [ -z "$SERVER_PID" ]; then
    echo "无法获取服务器PID"
    exit 1
fi

echo "服务器 PID: $SERVER_PID"
echo ""

# 内存监控函数
monitor_memory() {
    local pid=$1
    local duration=$2
    local output_file=$3
    
    echo "开始监控内存（PID: $pid，持续: $duration 秒）..."
    
    {
        echo "时间戳,RSS(MB),VSS(MB),线程数,连接数"
        local start_time=$(date +%s)
        local end_time=$((start_time + duration))
        
        while [ $(date +%s) -lt $end_time ]; do
            local timestamp=$(date +%s)
            
            # 获取内存信息（RSS, VSS）
            local rss_kb=0
            local vss_kb=0
            if [ -f "/proc/$pid/status" ]; then
                rss_kb=$(grep "^VmRSS:" /proc/$pid/status 2>/dev/null | awk '{print $2}' || echo "0")
                vss_kb=$(grep "^VmSize:" /proc/$pid/status 2>/dev/null | awk '{print $2}' || echo "0")
            fi
            
            # RSS 和 VSS 单位是 KB，转换为 MB
            local rss_mb=0
            local vss_mb=0
            if [ -n "$rss_kb" ] && [ "$rss_kb" != "0" ]; then
                rss_mb=$(echo "scale=2; $rss_kb / 1024" | bc 2>/dev/null || echo "0")
            fi
            if [ -n "$vss_kb" ] && [ "$vss_kb" != "0" ]; then
                vss_mb=$(echo "scale=2; $vss_kb / 1024" | bc 2>/dev/null || echo "0")
            fi
            
            # 统计线程数
            local thread_count=0
            if [ -d "/proc/$pid/task" ]; then
                thread_count=$(ls /proc/$pid/task 2>/dev/null | wc -l)
            fi
            
            # 统计当前连接数
            local conn_count=0
            conn_count=$(ss -tn 2>/dev/null | grep ":$PORT " | grep ESTAB | wc -l)
            
            echo "$timestamp,$rss_mb,$vss_mb,$thread_count,$conn_count"
            sleep 1
        done
    } > "$output_file"
}

# 计算平均内存
calc_avg_memory() {
    local file=$1
    local field=$2  # 2=RSS, 3=VSS
    awk -F',' "NR>1 && \$$field>0 {sum+=\$$field; count++} END {if(count>0) printf \"%.2f\", sum/count; else printf \"0\"}" "$file"
}

# 获取峰值内存
calc_max_memory() {
    local file=$1
    local field=$2
    awk -F',' "NR>1 && \$$field>0 {if(\$$field>max || max==\"\") max=\$$field} END {printf \"%.2f\", max+0}" "$file"
}

# 测试1：普通 HTTP QPS（无长连接）
echo "=========================================="
echo "测试1: 普通 HTTP QPS（无长连接背景）"
echo "=========================================="

BEFORE_CONN=$(ss -tn 2>/dev/null | grep ":$PORT " | grep ESTAB | wc -l)
echo "压测前连接数: $BEFORE_CONN"
echo ""

MEM_LOG1=$(mktemp)
monitor_memory $SERVER_PID 35 "$MEM_LOG1" &
MONITOR_PID1=$!

sleep 2

echo "运行 wrk 压测..."
WRK_OUT1=$(mktemp)
wrk -t$WRK_THREADS -c$WRK_CONNECTIONS -d$WRK_DURATION --latency "http://$HOST:$PORT/hello" > "$WRK_OUT1" 2>&1

sleep 2
kill $MONITOR_PID1 2>/dev/null
wait $MONITOR_PID1 2>/dev/null

AFTER_CONN=$(ss -tn 2>/dev/null | grep ":$PORT " | grep ESTAB | wc -l)
QPS1=$(grep "Requests/sec" "$WRK_OUT1" | awk '{print $2}')
AVG_RSS1=$(calc_avg_memory "$MEM_LOG1" 2)
AVG_VSS1=$(calc_avg_memory "$MEM_LOG1" 3)
MAX_RSS1=$(calc_max_memory "$MEM_LOG1" 2)
MAX_VSS1=$(calc_max_memory "$MEM_LOG1" 3)

echo ""
echo "结果:"
echo "  QPS: $QPS1"
echo "  平均内存: RSS=${AVG_RSS1}MB, VSS=${AVG_VSS1}MB"
echo "  峰值内存: RSS=${MAX_RSS1}MB, VSS=${MAX_VSS1}MB"
echo "  压测后连接数: $AFTER_CONN"
echo ""

# 等待连接清理
sleep 5

# 测试2：十万连接下的 QPS 和内存
echo "=========================================="
echo "测试2: 十万并发连接下的 QPS 和内存占用"
echo "=========================================="

# 检查ulimit
cur=$(ulimit -n 2>/dev/null); need=110000
if [ -z "$cur" ] || [ "$cur" -lt "$need" ]; then
    echo "⚠️  ulimit -n 不足: 当前 $cur，需要 >= $need"
    echo "   无法进行10万连接测试，跳过..."
    echo ""
    echo "=========================================="
    echo "最终对比"
    echo "=========================================="
    printf "%-20s %-15s %-20s %-20s\n" "场景" "QPS" "平均RSS(MB)" "峰值RSS(MB)"
    printf "%-20s %-15s %-20s %-20s\n" "------" "---" "----------" "----------"
    printf "%-20s %-15s %-20s %-20s\n" "普通HTTP" "$QPS1" "$AVG_RSS1" "$MAX_RSS1"
    printf "%-20s %-15s %-20s %-20s\n" "10万连接" "未测试" "-" "-"
    rm -f "$WRK_OUT1" "$MEM_LOG1"
    exit 0
fi

# 添加多IP
for i in 2 3 4; do
    ip addr show lo 2>/dev/null | grep -q "127.0.0.$i" || sudo ip addr add 127.0.0.$i/32 dev lo 2>/dev/null || true
done

echo "建立10万长连接..."
LOG=$(mktemp -d)
BEFORE_CONN2=$(ss -tn 2>/dev/null | grep ":$PORT " | grep ESTAB | wc -l)

# 记录建立连接前的内存
MEM_BEFORE=$(grep "^VmRSS:" /proc/$SERVER_PID/status 2>/dev/null | awk '{print $2}' || echo "0")
MEM_BEFORE_MB=$(echo "scale=2; $MEM_BEFORE / 1024" | bc 2>/dev/null || echo "0")
echo "建立连接前内存: RSS=${MEM_BEFORE_MB}MB"

for ip in 127.0.0.1 127.0.0.2 127.0.0.3 127.0.0.4; do
    "$BIN/concurrent_test" $HOST $PORT 25000 90 $ip > "$LOG/${ip}.log" 2>&1 &
done

echo "等待连接建立（90秒）..."
sleep 90

ESTABLISHED=$(ss -tn 2>/dev/null | grep ":$PORT " | grep ESTAB | wc -l)
ESTABLISHED=$((ESTABLISHED - BEFORE_CONN2))
echo "已建立连接数: $ESTABLISHED"

# 记录建立连接后的内存（维持连接时的内存）
MEM_AFTER_CONN=$(grep "^VmRSS:" /proc/$SERVER_PID/status 2>/dev/null | awk '{print $2}' || echo "0")
MEM_AFTER_CONN_MB=$(echo "scale=2; $MEM_AFTER_CONN / 1024" | bc 2>/dev/null || echo "0")
MEM_CONN_DELTA=$(echo "scale=2; $MEM_AFTER_CONN_MB - $MEM_BEFORE_MB" | bc 2>/dev/null || echo "0")
echo "维持连接时内存: RSS=${MEM_AFTER_CONN_MB}MB（增加 ${MEM_CONN_DELTA}MB）"

if [ "$ESTABLISHED" -lt 90000 ]; then
    echo "⚠️  警告: 只建立了 $ESTABLISHED 个连接，未达到10万"
fi

echo ""
echo "运行 wrk 压测（在10万连接基础上）..."

MEM_LOG2=$(mktemp)
monitor_memory $SERVER_PID 35 "$MEM_LOG2" &
MONITOR_PID2=$!

sleep 2

WRK_OUT2=$(mktemp)
wrk -t$WRK_THREADS -c$WRK_CONNECTIONS -d$WRK_DURATION --latency "http://$HOST:$PORT/hello" > "$WRK_OUT2" 2>&1

sleep 2
kill $MONITOR_PID2 2>/dev/null
wait $MONITOR_PID2 2>/dev/null

QPS2=$(grep "Requests/sec" "$WRK_OUT2" | awk '{print $2}')
AVG_RSS2=$(calc_avg_memory "$MEM_LOG2" 2)
AVG_VSS2=$(calc_avg_memory "$MEM_LOG2" 3)
MAX_RSS2=$(calc_max_memory "$MEM_LOG2" 2)
MAX_VSS2=$(calc_max_memory "$MEM_LOG2" 3)

echo ""
echo "清理10万连接..."
pkill -f concurrent_test
wait 2>/dev/null
rm -rf "$LOG"

sleep 5

# 输出对比结果
echo ""
echo "=========================================="
echo "最终对比结果"
echo "=========================================="
printf "%-20s %-15s %-20s %-20s %-20s\n" "场景" "QPS" "平均RSS(MB)" "峰值RSS(MB)" "连接数"
printf "%-20s %-15s %-20s %-20s %-20s\n" "------" "---" "----------" "----------" "------"
printf "%-20s %-15s %-20s %-20s %-20s\n" "普通HTTP" "$QPS1" "$AVG_RSS1" "$MAX_RSS1" "$AFTER_CONN"
printf "%-20s %-15s %-20s %-20s %-20s\n" "10万连接下" "$QPS2" "$AVG_RSS2" "$MAX_RSS2" "$ESTABLISHED"
echo ""

if [ -n "$QPS1" ] && [ -n "$QPS2" ]; then
    QPS_DIFF=$(echo "scale=2; ($QPS2 - $QPS1) / $QPS1 * 100" | bc 2>/dev/null || echo "0")
    RSS_DIFF=$(echo "scale=2; $AVG_RSS2 - $AVG_RSS1" | bc 2>/dev/null || echo "0")
    echo "变化:"
    echo "  QPS变化: ${QPS_DIFF}%"
    echo "  平均内存增加: ${RSS_DIFF}MB"
    echo "  维持10万连接内存增加: ${MEM_CONN_DELTA}MB"
fi

echo ""
echo "详细结果已保存:"
echo "  普通HTTP: wrk输出 -> $WRK_OUT1, 内存日志 -> $MEM_LOG1"
echo "  10万连接: wrk输出 -> $WRK_OUT2, 内存日志 -> $MEM_LOG2"
