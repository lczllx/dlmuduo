#!/bin/sh
set -e

# 等待 TCP 端口可达（带超时，避免永久卡死）
wait_tcp() {
    host="$1"
    port="$2"
    label="$3"
    max_retries="${4:-60}"

    i=0
    until nc -z "$host" "$port" 2>/dev/null; do
        i=$((i + 1))
        if [ "$i" -ge "$max_retries" ]; then
            echo "[entrypoint] ERROR: $label ($host:$port) 在 ${max_retries}s 内未就绪，退出"
            exit 1
        fi
        echo "[entrypoint] waiting for $label ($host:$port)... ${i}/${max_retries}"
        sleep 1
    done
    echo "[entrypoint] $label ($host:$port) is reachable"
}

# 校验必需变量（无默认值则报错退出）
if [ -z "${MYSQL_HOST:-}" ]; then
    echo "[entrypoint] ERROR: MYSQL_HOST 未设置"
    exit 1
fi
if [ -z "${REDIS_HOST:-}" ]; then
    echo "[entrypoint] ERROR: REDIS_HOST 未设置"
    exit 1
fi

wait_tcp "$MYSQL_HOST" "${MYSQL_PORT:-3306}" "MySQL"
wait_tcp "$REDIS_HOST" "${REDIS_PORT:-6379}" "Redis"

echo "[entrypoint] all dependencies ready, starting server"
exec ./shortener_server
