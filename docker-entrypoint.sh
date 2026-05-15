#!/bin/sh
set -e

# 等待 TCP 端口可达，不依赖 compose 版本的 healthcheck 行为
wait_tcp() {
    host="$1"
    port="$2"
    label="$3"

    until nc -z "$host" "$port" 2>/dev/null; do
        echo "[entrypoint] waiting for $label ($host:$port)..."
        sleep 1
    done
    echo "[entrypoint] $label ($host:$port) is reachable"
}

wait_tcp "$MYSQL_HOST" "${MYSQL_PORT:-3306}" "MySQL"
wait_tcp "$REDIS_HOST" "${REDIS_PORT:-6379}" "Redis"

echo "[entrypoint] all dependencies ready, starting server"
exec ./shortener_server
