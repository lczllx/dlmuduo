#!/bin/bash
# 配置系统参数以支持10万并发连接测试
# 路径: scripts/bench/setup_100k_limits.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== 配置10万并发连接所需的系统参数 ==="
echo ""

# 检查是否为root
if [ "$EUID" -ne 0 ]; then 
    echo "需要root权限，请使用 sudo 运行"
    echo "用法: sudo $SCRIPT_DIR/setup_100k_limits.sh"
    exit 1
fi

echo "当前系统参数："
echo "  net.core.somaxconn: $(sysctl -n net.core.somaxconn)"
echo "  net.ipv4.tcp_max_syn_backlog: $(sysctl -n net.ipv4.tcp_max_syn_backlog)"
echo "  net.ipv4.ip_local_port_range: $(sysctl -n net.ipv4.ip_local_port_range)"
echo ""

# 1. 增加监听队列长度（服务器端）
echo "1. 设置 net.core.somaxconn = 65535（监听队列最大长度）"
sysctl -w net.core.somaxconn=65535

# 2. 增加SYN队列长度（服务器端）
echo "2. 设置 net.ipv4.tcp_max_syn_backlog = 65535（SYN队列最大长度）"
sysctl -w net.ipv4.tcp_max_syn_backlog=65535

# 3. 扩大本地端口范围（客户端需要，wrk建立连接时用）
echo "3. 设置 net.ipv4.ip_local_port_range = 1024 65535（本地端口范围）"
sysctl -w net.ipv4.ip_local_port_range="1024 65535"

# 4. 增加TCP连接跟踪表大小（如果启用了conntrack）
if sysctl net.netfilter.nf_conntrack_max &>/dev/null; then
    echo "4. 设置 net.netfilter.nf_conntrack_max = 200000"
    sysctl -w net.netfilter.nf_conntrack_max=200000
fi

# 5. 增加TCP缓冲区（可选，但有助于性能）
echo "5. 设置TCP缓冲区大小"
sysctl -w net.core.rmem_max=16777216
sysctl -w net.core.wmem_max=16777216
sysctl -w net.ipv4.tcp_rmem="4096 87380 16777216"
sysctl -w net.ipv4.tcp_wmem="4096 65536 16777216"

echo ""
echo "=== 验证设置 ==="
echo "  net.core.somaxconn: $(sysctl -n net.core.somaxconn)"
echo "  net.ipv4.tcp_max_syn_backlog: $(sysctl -n net.ipv4.tcp_max_syn_backlog)"
echo "  net.ipv4.ip_local_port_range: $(sysctl -n net.ipv4.ip_local_port_range)"
echo ""

# 保存到配置文件（永久生效）
CONFIG_FILE="/etc/sysctl.d/99-muduo-100k.conf"
echo "保存配置到 $CONFIG_FILE（永久生效）..."
cat > "$CONFIG_FILE" << EOF
# muduo 10万并发连接配置
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.ipv4.ip_local_port_range = 1024 65535
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216
net.ipv4.tcp_rmem = 4096 87380 16777216
net.ipv4.tcp_wmem = 4096 65536 16777216
EOF

if sysctl net.netfilter.nf_conntrack_max &>/dev/null; then
    echo "net.netfilter.nf_conntrack_max = 200000" >> "$CONFIG_FILE"
fi

echo "✓ 配置已保存，重启后仍会生效"
echo ""
echo "=== 重要提示 ==="
echo "1. 确保 ulimit -n >= 110000（已在 limits.d 中配置）"
echo "2. 服务器和客户端都需要这些设置"
echo "3. 现在可以重新运行测试: $SCRIPT_DIR/test_qps_cpu.sh"
