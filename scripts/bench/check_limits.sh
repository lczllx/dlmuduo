#!/bin/bash
# 检查系统对 10 万并发的限制（2C2G 云服务器常见瓶颈）
# 路径: scripts/bench/check_limits.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$REPO_ROOT/bin"

echo "=== 10万连接限制检查 ==="
echo ""
echo "1. 进程 fd 限制 (ulimit -n):"
ulimit -n
echo "   (需 >= 110000，服务器和客户端都要)"
echo ""
echo "2. 系统总 fd 上限 (fs.file-max):"
cat /proc/sys/fs/file-max 2>/dev/null || echo "  无法读取"
echo "   (需 >= 200000，服务器+客户端共需约20万)"
echo ""
echo "3. 当前已用 fd 数:"
cat /proc/sys/fs/file-nr 2>/dev/null | awk '{print "  已分配:"$1" 空闲:"$2" 最大:"$3}'
echo ""
echo "4. 连接跟踪上限 (nf_conntrack_max，若存在):"
cat /proc/sys/net/netfilter/nf_conntrack_max 2>/dev/null || echo "  无/未加载 (无此限制)"
echo "  (若有值且<110000，可 sudo sysctl -w net.netfilter.nf_conntrack_max=200000)"
echo ""
echo "5. 云厂商限制: 多数 2C2G 实例对单机连接数有 ~6.5万 软限制"
echo ""
echo "=== 一键配置 10 万并发限制 ==="
echo "  调整 /etc/security/limits.d/ 或 limits.conf 中 nofile"
echo "  重新登录或新开终端后，ulimit -n 会生效"
echo ""
echo "=== 要测 10 万并发，可选 ==="
echo "  - QPS 压测: ./WebBench-master/webbench -c 1000 -t 30 http://127.0.0.1:8889/hello"
echo "    (2C2G 建议 -c 1000~2000，webbench 每客户端 fork 一进程)"
echo "  - 多机压测: 服务器放本机/其他机，4台客户端各执行:"
echo "    $BIN/concurrent_test <服务器IP> 8889 25000 60"
echo "  - 云端压测服务: k6 cloud、阿里云 PTS 等，分布式发起 10 万连接"
echo "  - 换环境: 物理机或 4C8G+ 云实例，再跑 $SCRIPT_DIR/run_qps_wrk.sh --100k"
