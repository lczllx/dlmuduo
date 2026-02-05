#!/bin/bash
# 正确的 QPS 压测脚本 - 使用 wrk (支持 HTTP keep-alive，复用连接)
# WebBench 每个请求新建连接，测的是「建连+处理」能力，无法反映 muduo 真实 QPS
#
# 用法: 
#   1. 安装 wrk: sudo apt install wrk
#   2. 先启动 ./bin/http_server (十万并发需用 run_server_for_100k.sh 启动以调高 ulimit)
#   3. ./run_qps_wrk.sh          # 普通压测: 8 线程 10000 连接
#   4. ./run_qps_wrk.sh --100k   # 十万并发压测: 先建立 10 万连接，再测 QPS

HOST=127.0.0.1
PORT=8889
# 4c8g 配置
WRK_THREADS=8
WRK_CONNECTIONS=10000
WRK_DURATION=30s

if ! command -v wrk &>/dev/null; then
    echo "请先安装 wrk: sudo apt install wrk"
    exit 1
fi

if ! ss -tlnp 2>/dev/null | grep -q ":$PORT "; then
    echo "请先启动 HTTP 服务: ./bin/http_server"
    echo "十万并发压测请用: ./run_http_server_for_100k.sh"
    exit 1
fi

# 十万并发模式
if [[ "$1" == "--100k" ]]; then
    # 十万连接+wrk 约需 11 万 fd，仅在不满足时报错
    cur=$(ulimit -n 2>/dev/null); need=110000
    if [ -z "$cur" ] || [ "$cur" -lt "$need" ]; then
      echo "十万并发需 ulimit -n >= $need，当前: $cur。请调整 limits.conf 或 limits.d 后重新登录"
      exit 1
    fi
    # 添加多 IP（突破单机端口限制）
    for i in 2 3 4; do
        ip addr show lo 2>/dev/null | grep -q "127.0.0.$i" || sudo ip addr add 127.0.0.$i/32 dev lo 2>/dev/null || true
    done

    echo "=== 10 万连接下 QPS 压测 (wrk + keep-alive) ==="
    echo "1. 后台建立 10 万连接并保持..."
    LOG=$(mktemp -d)
    for ip in 127.0.0.1 127.0.0.2 127.0.0.3 127.0.0.4; do
        ./bin/concurrent_test $HOST $PORT 25000 90 $ip > "$LOG/${ip}.log" 2>&1 &
    done

    echo "2. 等待连接建立 (约 90 秒)..."
    sleep 90

    echo "3. 运行 wrk 测 QPS ($WRK_THREADS 线程, $WRK_CONNECTIONS 连接, $WRK_DURATION)..."
    wrk -t$WRK_THREADS -c$WRK_CONNECTIONS -d$WRK_DURATION --latency "http://$HOST:$PORT/hello"

    echo ""
    echo "4. 连接保持任务仍在后台，可等待其自然结束"
    wait 2>/dev/null
    rm -rf "$LOG"
else
    echo "=== muduo QPS 压测 (wrk + keep-alive) ==="
    echo "4c8g: $WRK_THREADS 线程, $WRK_CONNECTIONS 连接, $WRK_DURATION"
    echo "十万并发压测请加参数: ./run_qps_wrk.sh --100k"
    echo ""
    wrk -t$WRK_THREADS -c$WRK_CONNECTIONS -d$WRK_DURATION --latency "http://$HOST:$PORT/hello"
fi
