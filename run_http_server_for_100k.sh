#!/bin/bash
# 以支持 10 万连接的 fd 限制启动 HTTP 服务（供十万并发 QPS 压测用）
# 用法: ./run_http_server_for_100k.sh
# 若当前 shell 的 ulimit 不足，需在 limits 里调高 nofile 后【重新登录】再运行本脚本

need=110000
cur=$(ulimit -n 2>/dev/null)
if [ -z "$cur" ] || [ "$cur" -lt "$need" ]; then
  # 先尝试在当前 shell 调高
  if ulimit -n "$need" 2>/dev/null; then
    echo "ulimit -n 已设为 $(ulimit -n)"
  else
    echo "无法调高 ulimit，当前: $cur (十万并发需 >= $need)"
    echo ""
    echo "请先调高 nofile 后重新登录："
    echo "  sudo bash -c 'printf \"* soft nofile $need\\n* hard nofile $need\\n\" > /etc/security/limits.d/99-nofile.conf'"
    echo "  然后退出当前 SSH/终端，重新登录，再执行 ./run_http_server_for_100k.sh"
    exit 1
  fi
else
  echo "ulimit -n 已满足: $(ulimit -n)"
fi
exec ./bin/http_server
