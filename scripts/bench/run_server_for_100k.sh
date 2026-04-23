#!/bin/bash
# 以支持 10 万连接的 fd 限制启动 Echo 测试服务器
# 用法: ./scripts/bench/run_server_for_100k.sh
# 注意: 若 ulimit 调高失败，需修改 /etc/security/limits.conf 后重新登录

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN="$REPO_ROOT/bin"

# 仅在当前限制不足时调高，不降低原有更大的限制
cur=$(ulimit -n 2>/dev/null); need=110000
if [ -n "$cur" ] && [ "$cur" -lt "$need" ]; then
  ulimit -n "$need" 2>/dev/null && echo "ulimit -n 已设为 $(ulimit -n)" || echo "无法调高 ulimit，当前: $cur"
else
  echo "ulimit -n 已满足: $(ulimit -n)"
fi
exec "$BIN/test"
