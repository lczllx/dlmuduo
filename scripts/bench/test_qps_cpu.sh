#!/bin/bash
# 测试1万和10万连接下的QPS和CPU利用率
# 路径相对于仓库根: scripts/bench/test_qps_cpu.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$REPO_ROOT/bin"

HOST=127.0.0.1
PORT=8889
# 与 run_qps_wrk.sh 一致：4核机用4线程，便于对比 QPS
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

# CPU监控函数
monitor_cpu() {
    local pid=$1
    local duration=$2
    local output_file=$3
    
    echo "开始监控 CPU（PID: $pid，持续: $duration 秒）..."
    
    {
        echo "时间戳,CPU使用率%,线程数"
        local start_time=$(date +%s)
        local end_time=$((start_time + duration))
        local prev_total=0
        local prev_idle=0
        
        # 读取初始CPU时间
        if [ -f /proc/stat ]; then
            local cpu_line=$(grep "^cpu " /proc/stat)
            prev_total=$(echo $cpu_line | awk '{sum=0; for(i=2;i<=NF;i++) sum+=$i; print sum}')
            prev_idle=$(echo $cpu_line | awk '{print $5}')
        fi
        
        while [ $(date +%s) -lt $end_time ]; do
            local timestamp=$(date +%s)
            
            # 获取进程CPU使用率（使用top，更准确）
            local cpu_usage=0
            if command -v top &>/dev/null; then
                cpu_usage=$(top -bn1 -p $pid 2>/dev/null | grep -E "^[ ]*$pid" | awk '{print $9}' | sed 's/%//' | head -1)
                cpu_usage=${cpu_usage:-0}
            fi
            
            # 统计线程数
            local thread_count=0
            if [ -d "/proc/$pid/task" ]; then
                thread_count=$(ls /proc/$pid/task 2>/dev/null | wc -l)
            fi
            
            echo "$timestamp,$cpu_usage,$thread_count"
            sleep 1
        done
    } > "$output_file"
}

# 计算平均CPU使用率
calc_avg_cpu() {
    local file=$1
    awk -F',' 'NR>1 {sum+=$2; count++} END {if(count>0) printf "%.2f", sum/count; else printf "0"}' "$file"
}

# 测试1：1万连接（普通压测）
echo "=========================================="
echo "测试1: 1万连接压测"
echo "=========================================="

BEFORE_CONN=$(ss -tn 2>/dev/null | grep ":$PORT " | grep ESTAB | wc -l)
echo "压测前连接数: $BEFORE_CONN"
echo ""

CPU_LOG1=$(mktemp)
monitor_cpu $SERVER_PID 35 "$CPU_LOG1" &
MONITOR_PID1=$!

sleep 2  # 等待监控启动

echo "运行 wrk 压测..."
WRK_OUT1=$(mktemp)
wrk -t$WRK_THREADS -c$WRK_CONNECTIONS -d$WRK_DURATION --latency "http://$HOST:$PORT/hello" > "$WRK_OUT1" 2>&1

sleep 2  # 等待监控结束
kill $MONITOR_PID1 2>/dev/null
wait $MONITOR_PID1 2>/dev/null

AFTER_CONN=$(ss -tn 2>/dev/null | grep ":$PORT " | grep ESTAB | wc -l)
QPS1=$(grep "Requests/sec" "$WRK_OUT1" | awk '{print $2}')
AVG_CPU1=$(calc_avg_cpu "$CPU_LOG1")

echo ""
echo "结果:"
echo "  QPS: $QPS1"
echo "  平均CPU利用率: ${AVG_CPU1}%"
echo "  压测后连接数: $AFTER_CONN"
echo ""

# 等待连接清理
sleep 5

# 测试2：10万连接压测
echo "=========================================="
echo "测试2: 10万连接压测"
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
    echo "1万连接: QPS=$QPS1, CPU=${AVG_CPU1}%"
    echo "10万连接: 未测试（ulimit不足）"
    rm -f "$WRK_OUT1" "$CPU_LOG1"
    exit 0
fi

# 添加多IP
for i in 2 3 4; do
    ip addr show lo 2>/dev/null | grep -q "127.0.0.$i" || sudo ip addr add 127.0.0.$i/32 dev lo 2>/dev/null || true
done

echo "建立10万长连接..."
LOG=$(mktemp -d)
BEFORE_CONN2=$(ss -tn 2>/dev/null | grep ":$PORT " | grep ESTAB | wc -l)

for ip in 127.0.0.1 127.0.0.2 127.0.0.3 127.0.0.4; do
    "$BIN/concurrent_test" $HOST $PORT 25000 90 $ip > "$LOG/${ip}.log" 2>&1 &
done

echo "等待连接建立（90秒）..."
sleep 90

ESTABLISHED=$(ss -tn 2>/dev/null | grep ":$PORT " | grep ESTAB | wc -l)
ESTABLISHED=$((ESTABLISHED - BEFORE_CONN2))
echo "已建立连接数: $ESTABLISHED"

if [ "$ESTABLISHED" -lt 90000 ]; then
    echo "⚠️  警告: 只建立了 $ESTABLISHED 个连接，未达到10万"
fi

echo ""
echo "运行 wrk 压测（在10万连接基础上）..."

CPU_LOG2=$(mktemp)
monitor_cpu $SERVER_PID 35 "$CPU_LOG2" &
MONITOR_PID2=$!

sleep 2

WRK_OUT2=$(mktemp)
wrk -t$WRK_THREADS -c$WRK_CONNECTIONS -d$WRK_DURATION --latency "http://$HOST:$PORT/hello" > "$WRK_OUT2" 2>&1

sleep 2
kill $MONITOR_PID2 2>/dev/null
wait $MONITOR_PID2 2>/dev/null

QPS2=$(grep "Requests/sec" "$WRK_OUT2" | awk '{print $2}')
AVG_CPU2=$(calc_avg_cpu "$CPU_LOG2")

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
printf "%-15s %-15s %-15s\n" "连接数" "QPS" "平均CPU利用率"
printf "%-15s %-15s %-15s\n" "------" "---" "------------"
printf "%-15s %-15s %-15s%%\n" "1万连接" "$QPS1" "$AVG_CPU1"
printf "%-15s %-15s %-15s%%\n" "10万连接" "$QPS2" "$AVG_CPU2"
echo ""

if [ -n "$QPS1" ] && [ -n "$QPS2" ]; then
    QPS_DIFF=$(echo "scale=2; ($QPS2 - $QPS1) / $QPS1 * 100" | bc)
    CPU_DIFF=$(echo "scale=2; $AVG_CPU2 - $AVG_CPU1" | bc)
    echo "变化:"
    echo "  QPS变化: ${QPS_DIFF}%"
    echo "  CPU利用率变化: ${CPU_DIFF}%"
fi

echo ""
echo "详细结果已保存:"
echo "  1万连接: wrk输出 -> $WRK_OUT1, CPU日志 -> $CPU_LOG1"
echo "  10万连接: wrk输出 -> $WRK_OUT2, CPU日志 -> $CPU_LOG2"

# 清理临时文件（可选，注释掉以保留日志）
# rm -f "$WRK_OUT1" "$WRK_OUT2" "$CPU_LOG1" "$CPU_LOG2"
