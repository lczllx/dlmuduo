#!/bin/bash
# 以支持 10 万连接的 fd 限制启动 HTTP 服务（供十万并发 QPS 压测用）
# 用法: ./run_http_server_for_100k.sh
# 注意: 若 ulimit 调高失败，需修改 /etc/security/limits.conf 后重新登录

# 仅在当前限制不足时调高，不降低原有更大的限制
cur=$(ulimit -n 2>/dev/null); need=120000
if [ -n "$cur" ] && [ "$cur" -lt "$need" ]; then
  ulimit -n "$need" 2>/dev/null && echo "ulimit -n 已设为 $(ulimit -n)" || echo "无法调高 ulimit，当前: $cur (十万并发需 >= $need)"
else
  echo "ulimit -n 已满足: $(ulimit -n)"
fi
exec ./bin/http_server
