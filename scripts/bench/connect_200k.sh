#!/bin/bash
# 建立 20 万长连接并保持，供 wrk 压测使用
# 用法: ./connect_200k.sh <服务器IP> <端口> [保持秒数] [客户端base_IP]
# 示例: ./connect_200k.sh 192.168.175.86 8889 300 192.168.175.87

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$REPO_ROOT/bin"

SERVER_IP="${1:?用法: $0 <服务器IP> <端口> [保持秒数] [客户端真实IP]}"
PORT="${2:-8889}"
HOLD_SEC="${3:-300}"
BASE_IP="${4:?必须指定客户端真实IP（eth0上的主IP）}"

# 从真实 IP 推虚拟 IP：最后一段 +100
IP_PREFIX="${BASE_IP%.*}"
IP_END="${BASE_IP##*.}"
IP2="$IP_PREFIX.$((IP_END + 10))"
IP3="$IP_PREFIX.$((IP_END + 20))"
IP4="$IP_PREFIX.$((IP_END + 30))"

CONNS_PER_PROC=50000
TOTAL=$(($CONNS_PER_PROC * 4))

echo "=========================================="
echo " 20 万长连接压测工具"
echo "=========================================="
echo "服务器:    $SERVER_IP:$PORT"
echo "保持时长:  ${HOLD_SEC}s"
echo "IP 分配:   $BASE_IP / $IP2 / $IP3 / $IP4"
echo "每进程:    $CONNS_PER_PROC 连接"
echo "总计:      $TOTAL 连接"
echo "=========================================="

# ---- 前置检查 ----
if [ "$(ulimit -n)" -lt 220000 ]; then
    echo "调高 ulimit..."
    ulimit -n 220000 || {
        echo "ERROR: ulimit -n 220000 失败，请先调 hard limit"
        exit 1
    }
fi
echo "ulimit: $(ulimit -n)"

if ! command -v "$BIN/concurrent_test" &>/dev/null && [ ! -x "$BIN/concurrent_test" ]; then
    echo "concurrent_test 不存在，正在编译..."
    cd "$REPO_ROOT/build" && cmake .. && make -j$(nproc) concurrent_test
fi
echo "concurrent_test: OK"

# ---- 添加虚拟 IP ----
add_ip() {
    local ip="$1"
    if ip addr show eth0 | grep -q "$ip"; then
        echo "IP $ip 已存在，跳过"
        return 0
    fi
    echo "添加虚拟 IP: $ip"
    sudo ip addr add "$ip/16" dev eth0 || {
        echo "ERROR: 添加 $ip 失败"
        return 1
    }
}

for ip in "$IP2" "$IP3" "$IP4"; do
    add_ip "$ip"
done

# 验证连通性
echo ""
echo "验证网络连通性..."
for ip in "$BASE_IP" "$IP2" "$IP3" "$IP4"; do
    if ping -I "$ip" -c1 -W2 "$SERVER_IP" &>/dev/null; then
        echo "  $ip -> $SERVER_IP  OK"
    else
        echo "  $ip -> $SERVER_IP  FAIL（将跳过此 IP）"
    fi
done

# ---- 启动连接进程 ----
echo ""
echo "启动 4 个连接进程..."
rm -f /tmp/conn_*.log

"$BIN/concurrent_test" "$SERVER_IP" "$PORT" "$CONNS_PER_PROC" "$HOLD_SEC" "$BASE_IP" > /tmp/conn_1.log 2>&1 &
PID1=$!
"$BIN/concurrent_test" "$SERVER_IP" "$PORT" "$CONNS_PER_PROC" "$HOLD_SEC" "$IP2" > /tmp/conn_2.log 2>&1 &
PID2=$!
"$BIN/concurrent_test" "$SERVER_IP" "$PORT" "$CONNS_PER_PROC" "$HOLD_SEC" "$IP3" > /tmp/conn_3.log 2>&1 &
PID3=$!
"$BIN/concurrent_test" "$SERVER_IP" "$PORT" "$CONNS_PER_PROC" "$HOLD_SEC" "$IP4" > /tmp/conn_4.log 2>&1 &
PID4=$!

echo "进程 PID: $PID1 $PID2 $PID3 $PID4"

# ---- 等待连接建立 ----
cleanup() {
    echo ""
    echo "清理连接进程..."
    kill $PID1 $PID2 $PID3 $PID4 2>/dev/null
    wait $PID1 $PID2 $PID3 $PID4 2>/dev/null
    echo "清理完成"
}
trap cleanup EXIT INT TERM

echo ""
echo "等待连接建立（最多 120 秒）..."
WAIT_SEC=0
while [ $WAIT_SEC -lt 120 ]; do
    sleep 10
    WAIT_SEC=$((WAIT_SEC + 10))

    # 统计各进程进度
    echo ""
    echo "--- ${WAIT_SEC}s ---"
    for log in /tmp/conn_1.log /tmp/conn_2.log /tmp/conn_3.log /tmp/conn_4.log; do
        if [ -f "$log" ]; then
            last=$(grep -E "已发起|最终统计|已连接" "$log" | tail -1)
            echo "  $(basename $log): $last"
        fi
    done

    # 检查是否全部完成
    done_count=0
    for log in /tmp/conn_1.log /tmp/conn_2.log /tmp/conn_3.log /tmp/conn_4.log; do
        if grep -q "最终统计" "$log" 2>/dev/null; then
            done_count=$((done_count + 1))
        fi
    done
    if [ $done_count -eq 4 ]; then
        echo ""
        echo "全部进程连接建立完成！"
        break
    fi
done

# ---- 汇总 ----
echo ""
echo "=========================================="
echo " 连接汇总"
echo "=========================================="
total_conn=0
for log in /tmp/conn_1.log /tmp/conn_2.log /tmp/conn_3.log /tmp/conn_4.log; do
    conn=$(grep "成功连接:" "$log" | grep -oP '\d+' | head -1)
    echo "  $(basename $log): ${conn:-0} 连接"
    total_conn=$((total_conn + ${conn:-0}))
done
echo "  总计: $total_conn"
echo ""

if [ "$total_conn" -lt 180000 ]; then
    echo "WARNING: 实际连接数 $total_conn < 180000，可能未达 20 万目标"
    echo "原因可能是: ulimit 不足、服务器 fd 不够、虚拟 IP 不通"
else
    echo "连接建立成功！现在可以在另一个终端执行压测："
    echo "  wrk -t4 -c10000 -d60s --latency http://$SERVER_IP:$PORT/hello"
fi

echo ""
echo "长连接将保持 ${HOLD_SEC} 秒，或按 Ctrl+C 提前结束"
echo "剩余时间中..."
sleep $HOLD_SEC
echo "时间到。"
