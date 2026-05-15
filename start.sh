#!/bin/bash
set -e

# ============================================================
# start.sh — 纯 docker 命令启动全部服务，零 compose 依赖
# 使用方式: ./start.sh [--build]
#           ./start.sh --stop  清理所有容器/网络/卷
# ============================================================

NETWORK="shortener-net"
MYSQL_CONTAINER="shortener-mysql"
REDIS_CONTAINER="shortener-redis"
APP_CONTAINER="shortener-app"
IMAGE="shortener-app:latest"
VOLUME="shortener-mysql-data"
APP_PORT="${APP_PORT:-8889}"

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ---------- 清理 ----------
if [ "$1" = "--stop" ]; then
    echo "==> 停止并移除所有容器"
    docker rm -f "$APP_CONTAINER" "$REDIS_CONTAINER" "$MYSQL_CONTAINER" 2>/dev/null || true
    echo "==> 移除网络"
    docker network rm "$NETWORK" 2>/dev/null || true
    echo "==> 如果要清空 MySQL 数据卷: docker volume rm $VOLUME"
    echo "==> 清理完成"
    exit 0
fi

# ---------- 前置检查 ----------
if ! docker info >/dev/null 2>&1; then
    echo "ERROR: Docker 未启动或无权限"
    exit 1
fi

# ---------- 网络 ----------
if ! docker network inspect "$NETWORK" >/dev/null 2>&1; then
    echo "==> 创建网络 $NETWORK"
    docker network create "$NETWORK"
fi

# ---------- 数据卷 ----------
if ! docker volume inspect "$VOLUME" >/dev/null 2>&1; then
    echo "==> 创建数据卷 $VOLUME"
    docker volume create "$VOLUME"
fi

# ---------- MySQL ----------
if docker inspect "$MYSQL_CONTAINER" >/dev/null 2>&1; then
    echo "==> MySQL 容器已存在，跳过创建"
else
    echo "==> 启动 MySQL"
    docker run -d \
        --name "$MYSQL_CONTAINER" \
        --network "$NETWORK" \
        -e MYSQL_ROOT_PASSWORD=shortener \
        -p 3307:3306 \
        -v "$VOLUME:/var/lib/mysql" \
        -v "$PROJECT_DIR/net/http/shortener/init.sql:/docker-entrypoint-initdb.d/init.sql" \
        --restart unless-stopped \
        mysql:8.0
fi

# ---------- Redis ----------
if docker inspect "$REDIS_CONTAINER" >/dev/null 2>&1; then
    echo "==> Redis 容器已存在，跳过创建"
else
    echo "==> 启动 Redis"
    docker run -d \
        --name "$REDIS_CONTAINER" \
        --network "$NETWORK" \
        --restart unless-stopped \
        redis:7-alpine
fi

# ---------- 等待依赖就绪 ----------
echo "==> 等待 MySQL 就绪"
until docker exec "$MYSQL_CONTAINER" mysqladmin ping -h localhost -uroot -pshortener --silent 2>/dev/null; do
    sleep 1
done
echo "    MySQL 已就绪"

echo "==> 等待 Redis 就绪"
until docker exec "$REDIS_CONTAINER" redis-cli ping 2>/dev/null | grep -q PONG; do
    sleep 1
done
echo "    Redis 已就绪"

# ---------- 构建镜像 ----------
if [ "$1" = "--build" ] || ! docker inspect "$IMAGE" >/dev/null 2>&1; then
    echo "==> 构建应用镜像"
    docker build -t "$IMAGE" -f "$PROJECT_DIR/Dockerfile" "$PROJECT_DIR"
fi

# ---------- 启动应用 ----------
if docker inspect "$APP_CONTAINER" >/dev/null 2>&1; then
    echo "==> 应用容器已存在，重新启动"
    docker rm -f "$APP_CONTAINER" 2>/dev/null || true
fi

echo "==> 启动应用"
docker run -d \
    --name "$APP_CONTAINER" \
    --network "$NETWORK" \
    -p "$APP_PORT:8889" \
    -e MYSQL_HOST="$MYSQL_CONTAINER" \
    -e MYSQL_PORT=3306 \
    -e MYSQL_USER=root \
    -e MYSQL_PASS=shortener \
    -e MYSQL_DB=shortener \
    -e REDIS_HOST="$REDIS_CONTAINER" \
    -e REDIS_PORT=6379 \
    -e BASE_URL="http://localhost:$APP_PORT/" \
    --restart unless-stopped \
    "$IMAGE"

echo ""
echo "=============================================="
echo "  全部服务已启动"
echo "  App:  http://localhost:$APP_PORT"
echo "  MySQL: localhost:3307"
echo ""
echo "  查看日志: docker logs -f $APP_CONTAINER"
echo "  停止:     ./start.sh --stop"
echo "=============================================="
