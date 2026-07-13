#!/usr/bin/env bash
# ============================================================
# dlmuduo — Latency 分位数 + 并发梯度性能测试
# 前置条件: 已编译 muduo, wrk2 已安装 (https://github.com/giltene/wrk2)
# 用法: bash run_latency_bench.sh [服务器IP] [端口]
# 默认: 127.0.0.1:8080
# 输出: 终端表格 + /tmp/muduo_bench_result.txt
# ============================================================
set -e

# ====== 配置 ======
SERVER_IP="${1:-127.0.0.1}"
SERVER_PORT="${2:-8080}"
RESULT_FILE="/tmp/muduo_bench_result.txt"
WRK2="$(command -v wrk2 2>/dev/null || echo '')"

# 颜色
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# ====== 检查 wrk2 ======
check_wrk2() {
    if [ -z "$WRK2" ]; then
        echo -e "${YELLOW}[WARN] 未安装 wrk2，尝试使用 wrk (无 latency 分位数)${NC}"
        WRK2="$(command -v wrk 2>/dev/null || echo '')"
        if [ -z "$WRK2" ]; then
            echo -e "${YELLOW}[ERROR] wrk 也未安装。请安装:${NC}"
            echo "  git clone https://github.com/giltene/wrk2.git && cd wrk2 && make"
            exit 1
        fi
        USE_WRK2=false
    else
        USE_WRK2=true
    fi
}

check_wrk2

# ====== 检查服务器 ======
echo -e "${BOLD}${CYAN}"
echo "╔══════════════════════════════════════════════════════════╗"
echo "║       dlmuduo — Latency 分位数性能测试                  ║"
echo "║       目标: ${SERVER_IP}:${SERVER_PORT}                              ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo -e "${NC}"

HTTP_URL="http://${SERVER_IP}:${SERVER_PORT}/hello"

# 快速连通性检查
if ! curl -s -o /dev/null -w "%{http_code}" --connect-timeout 3 "$HTTP_URL" 2>/dev/null | grep -q "200"; then
    echo -e "${YELLOW}[WARN] 无法连接到 $HTTP_URL，请确保:${NC}"
    echo "  cd muduo && mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j\$(nproc)"
    echo "  ../bin/http_server &"
    echo "  或: docker run -p 8080:8080 lcz-muduo"
    echo ""
    echo -e "${YELLOW}继续运行测试 (如果服务器没启动，所有测试会失败)${NC}"
    echo ""
fi

> "$RESULT_FILE"

# ====== 测试函数 ======
# 吐出一个整洁的延迟行
run_wrk() {
    local desc="$1" conns="$2" threads="$3" duration="$4" extra_args="$5"
    local output
    echo -e "\n${BOLD}${GREEN}[测试] $desc (c${conns} t${threads} ${duration}s)${NC}"

    if [ "$USE_WRK2" = true ]; then
        # wrk2: 恒定速率压测，输出协调遗漏(coordinated omission)修正后的分位数
        # -R 用一个大值(99999999)模拟不限制速率
        output=$($WRK2 -t"$threads" -c"$conns" -d"${duration}s" -R99999999 --latency $extra_args "$HTTP_URL" 2>&1 || true)
    else
        # wrk 不支持 latency 分位数，仅输出 avg/stdev/max
        output=$($WRK2 -t"$threads" -c"$conns" -d"${duration}s" --latency $extra_args "$HTTP_URL" 2>&1 || true)
    fi

    echo "$output" | tail -20
    echo "$output"
}

# ====== 开始测试 ======

echo "" | tee -a "$RESULT_FILE"
echo "=== dlmuduo 性能测试结果 ===" | tee -a "$RESULT_FILE"
echo "环境: $(nproc 2>/dev/null || echo '?')C  $(free -h | awk '/^Mem:/{print $2}')" | tee -a "$RESULT_FILE"
echo "目标: $HTTP_URL" | tee -a "$RESULT_FILE"
echo "工具: $([ "$USE_WRK2" = true ] && echo 'wrk2 (latency 分位数)' || echo 'wrk (仅 avg)')" | tee -a "$RESULT_FILE"
echo "" | tee -a "$RESULT_FILE"

# ---- 测试 1: 低并发 latency 基线 ----
echo "" | tee -a "$RESULT_FILE"
echo "--- 1. 低并发 Latency 基线 (c100) ---" | tee -a "$RESULT_FILE"
output1=$(run_wrk "低并发基线" 100 4 30 "")
reqs=$(echo "$output1" | grep "Requests/sec" | awk '{print $2}')
avg=$(echo "$output1"   | grep "Latency" | head -1 | awk '{print $2}')
stdev=$(echo "$output1"  | grep "Latency" | head -1 | awk '{print $3}')
# wrk2 输出 50%/90%/99%/99.9% 分位
p50_line=$(echo "$output1"  | grep "50.000%" || echo "N/A")
p99_line=$(echo "$output1"  | grep "99.000%" || echo "N/A")
p999_line=$(echo "$output1" | grep "99.900%" || echo "N/A")
echo "QPS: $reqs, Avg: $avg, Stdev: $stdev" | tee -a "$RESULT_FILE"
echo "P50:  $p50_line"   | tee -a "$RESULT_FILE"
echo "P99:  $p99_line"   | tee -a "$RESULT_FILE"
echo "P999: $p999_line"  | tee -a "$RESULT_FILE"

# ---- 测试 2: 中等并发 ----
echo "" | tee -a "$RESULT_FILE"
echo "--- 2. 中等并发 (c1000) ---" | tee -a "$RESULT_FILE"
output2=$(run_wrk "中等并发" 1000 4 30 "")
reqs=$(echo "$output2" | grep "Requests/sec" | awk '{print $2}')
p50_line=$(echo "$output2"  | grep "50.000%" || echo "N/A")
p99_line=$(echo "$output2"  | grep "99.000%" || echo "N/A")
p999_line=$(echo "$output2" | grep "99.900%" || echo "N/A")
echo "QPS: $reqs" | tee -a "$RESULT_FILE"
echo "P50:  $p50_line"   | tee -a "$RESULT_FILE"
echo "P99:  $p99_line"   | tee -a "$RESULT_FILE"
echo "P999: $p999_line"  | tee -a "$RESULT_FILE"

# ---- 测试 3: 高并发 ----
echo "" | tee -a "$RESULT_FILE"
echo "--- 3. 高并发 (c5000) ---" | tee -a "$RESULT_FILE"
output3=$(run_wrk "高并发" 5000 4 30 "")
reqs=$(echo "$output3" | grep "Requests/sec" | awk '{print $2}')
p50_line=$(echo "$output3"  | grep "50.000%" || echo "N/A")
p99_line=$(echo "$output3"  | grep "99.000%" || echo "N/A")
p999_line=$(echo "$output3" | grep "99.900%" || echo "N/A")
echo "QPS: $reqs" | tee -a "$RESULT_FILE"
echo "P50:  $p50_line"   | tee -a "$RESULT_FILE"
echo "P99:  $p99_line"   | tee -a "$RESULT_FILE"
echo "P999: $p999_line"  | tee -a "$RESULT_FILE"

# ---- 测试 4: 极限并发 ----
echo "" | tee -a "$RESULT_FILE"
echo "--- 4. 极限并发 (c10000) ---" | tee -a "$RESULT_FILE"
output4=$(run_wrk "极限并发" 10000 4 30 "")
reqs=$(echo "$output4" | grep "Requests/sec" | awk '{print $2}')
p50_line=$(echo "$output4"  | grep "50.000%" || echo "N/A")
p99_line=$(echo "$output4"  | grep "99.000%" || echo "N/A")
p999_line=$(echo "$output4" | grep "99.900%" || echo "N/A")
echo "QPS: $reqs" | tee -a "$RESULT_FILE"
echo "P50:  $p50_line"   | tee -a "$RESULT_FILE"
echo "P99:  $p99_line"   | tee -a "$RESULT_FILE"
echo "P999: $p999_line"  | tee -a "$RESULT_FILE"

# ---- 测试 5: 短连接测试 (Connection: close) ----
echo "" | tee -a "$RESULT_FILE"
echo "--- 5. 短连接 (Connection: close, c100) ---" | tee -a "$RESULT_FILE"
output5=$(run_wrk "短连接" 100 4 30 "-H 'Connection: close'")
reqs=$(echo "$output5" | grep "Requests/sec" | awk '{print $2}')
echo "QPS: $reqs" | tee -a "$RESULT_FILE"

# ====== 总结 ======
echo ""
echo -e "${BOLD}${CYAN}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${CYAN}║  测试完成！结果已保存到: ${RESULT_FILE}${NC}"
echo -e "${BOLD}${CYAN}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${YELLOW}简历数据提取指南:${NC}"
echo "  1. 取 c1000 下的 QPS 和 P99 作为主要性能指标"
echo "  2. 取 c10000 下的数据证明高并发下性能不退化"
echo "  3. 对比 keep-alive vs short-connection 的 QPS 差异"
echo "  4. 如果 P999 过大 (长尾), 记为已知优化点"
echo ""
echo -e "${YELLOW}额外测试 (手动跑):${NC}"
echo "  # 20万长连接驻留 + 内存观测"
echo "  bash scripts/bench/run_server_for_100k.sh"
echo "  bash scripts/bench/test_100k_qps_memory.sh"
echo ""
echo -e "${YELLOW}获取 wrk2 (如果没装):${NC}"
echo "  git clone https://github.com/giltene/wrk2.git"
echo "  cd wrk2 && make -j\$(nproc)"
echo "  sudo cp wrk /usr/local/bin/wrk2"
