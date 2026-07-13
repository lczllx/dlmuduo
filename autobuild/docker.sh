#!/usr/bin/env bash
# =============================================================================
# Docker 构建与部署脚本 — dlmuduo URL Shortener
# =============================================================================
# 用法:（从项目根目录执行，或任意位置给出脚本绝对/相对路径均可）
#   bash autobuild/docker.sh doctor
#   bash autobuild/docker.sh setup
#   bash autobuild/docker.sh build
#   bash autobuild/docker.sh compose
#   bash autobuild/docker.sh clean
# =============================================================================
set -e

# 自动定位项目根目录，从任何位置执行均可
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"
_ME="bash ${BASH_SOURCE[0]}"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BOLD='\033[1m'
NC='\033[0m'

ok()  { echo -e "  ${GREEN}[OK]${NC} $1"; }
warn(){ echo -e "  ${YELLOW}[!]${NC} $1"; }
err() { echo -e "  ${RED}[X]${NC} $1"; }

# ====== 检测构建参数（代理/网络） ======
# Docker daemon 会将 HTTP_PROXY 注入所有容器。
# 如果代理在 127.0.0.1，容器必须 --network host 才能访问；
# 但如果代理未运行，需要 unset proxy env 让 apk 直连外网。
_detect_build_flags() {
    local flags="DOCKER_BUILDKIT=0 docker build"
    local proxy_port
    proxy_port=$(systemctl show docker --property=Environment 2>/dev/null | grep -oP 'HTTPS?_PROXY=http://127\.0\.0\.1:\K\d+' | head -1 || true)
    if [ -n "$proxy_port" ]; then
        flags="$flags --network host"
        if ! ss -tlnp 2>/dev/null | grep -q ":$proxy_port " && ! sudo -n ss -tlnp 2>/dev/null | grep -q ":$proxy_port "; then
            flags="$flags --build-arg HTTP_PROXY= --build-arg http_proxy= --build-arg HTTPS_PROXY= --build-arg https_proxy="
        fi
    fi
    echo "$flags"
}

_build_flags_info() {
    local proxy_port
    proxy_port=$(systemctl show docker --property=Environment 2>/dev/null | grep -oP 'HTTPS?_PROXY=http://127\.0\.0\.1:\K\d+' | head -1 || true)
    if [ -n "$proxy_port" ]; then
        if ! ss -tlnp 2>/dev/null | grep -q ":$proxy_port " && ! sudo -n ss -tlnp 2>/dev/null | grep -q ":$proxy_port "; then
            warn "代理 127.0.0.1:$proxy_port 未运行，构建时 unset proxy 直连外网"
        else
            ok "代理 127.0.0.1:$proxy_port 运行中，构建使用 --network host"
        fi
    fi
}

# ====== 封装 docker compose ======
_compose() {
    if command -v docker >/dev/null && docker compose version >/dev/null 2>&1; then
        docker compose "$@"
    elif command -v docker-compose >/dev/null 2>&1; then
        warn "使用旧版 docker-compose v1，建议: sudo apt install docker-compose-plugin"
        docker-compose "$@"
    else
        err "未找到 docker compose（v1/v2 均不可用）"
        echo "  安装 v2: sudo apt install docker-compose-plugin"
        exit 1
    fi
}

# ====== 诊断 ======
doctor() {
    echo -e "${BOLD}Docker 环境诊断${NC}"
    echo ""

    # 1. Docker
    if command -v docker >/dev/null 2>&1; then
        ok "Docker 已安装 ($(docker --version 2>/dev/null | head -1))"
    else
        err "Docker 未安装 → sudo apt install docker.io"
    fi

    # 2. 权限
    if docker info >/dev/null 2>&1; then
        ok "Docker 权限正常"
    else
        err "Docker 权限不足 → sudo usermod -aG docker \$USER && newgrp docker"
    fi

    # 3. Docker Compose
    if command -v docker >/dev/null && docker compose version >/dev/null 2>&1; then
        ok "Docker Compose v2: $(docker compose version --short 2>/dev/null)"
    elif command -v docker-compose >/dev/null 2>&1; then
        warn "Docker Compose v1 (旧版建议升级)"
    else
        err "Docker Compose 未安装 → sudo apt install docker-compose-plugin"
    fi

    # 4. 网络可达性
    if timeout 3 bash -c "echo >/dev/tcp/registry-1.docker.io/443" 2>/dev/null; then
        ok "Docker Hub 直连可达"
    elif systemctl show docker --property=Environment 2>/dev/null | grep -q 'HTTPS\?_PROXY'; then
        ok "Docker daemon 已配置代理"
        warn "构建将使用 --network host 模式（代理在 127.0.0.1）"
    else
        warn "Docker Hub 不可达，且未检测到代理配置"
        echo "      → $_ME setup 自动检测并配置"
    fi

    # 5. 代理端口检测
    local proxy_port
    proxy_port=$(systemctl show docker --property=Environment 2>/dev/null | grep -oP '127\.0\.0\.1:\K\d+' | head -1 || true)
    if [ -n "$proxy_port" ]; then
        if ss -tlnp 2>/dev/null | grep -q "$proxy_port" || sudo -n ss -tlnp 2>/dev/null | grep -q "$proxy_port"; then
            ok "代理 127.0.0.1:$proxy_port 运行中"
        else
            warn "代理 127.0.0.1:$proxy_port 未运行 — 构建时将 unset proxy 直连外网"
        fi
    fi

    # 6. 项目文件
    [ -f "$PROJECT_ROOT/Dockerfile" ]     && ok "Dockerfile 存在"      || err "缺少 Dockerfile"
    [ -f "$PROJECT_ROOT/docker-compose.yml" ] && ok "docker-compose.yml 存在" || err "缺少 docker-compose.yml"

    echo ""
    echo -e "${YELLOW}推荐: $_ME setup  ← 自动检测环境并完成首次部署${NC}"
}

# ====== 一键 setup（诊断 → 构建 → 启动） ======
setup() {
    echo -e "${BOLD}Docker 环境自动部署${NC}"
    echo ""

    if ! command -v docker >/dev/null 2>&1; then
        err "请先安装 Docker: sudo apt install docker.io docker-compose-plugin"
        exit 1
    fi

    # 检测是否需要配置镜像源
    if ! timeout 3 bash -c "echo >/dev/tcp/registry-1.docker.io/443" 2>/dev/null; then
        if ! systemctl show docker --property=Environment 2>/dev/null | grep -q 'HTTPS\?_PROXY'; then
            echo -e "${GREEN}[setup] Docker Hub 不可达，配置国内镜像源 ...${NC}"
            sudo mkdir -p /etc/docker
            if [ ! -f /etc/docker/daemon.json ] || ! sudo grep -q 'registry-mirrors' /etc/docker/daemon.json 2>/dev/null; then
                sudo tee /etc/docker/daemon.json <<'EOF'
{
  "registry-mirrors": [
    "https://registry.cn-hangzhou.aliyuncs.com",
    "https://docker.mirrors.ustc.edu.cn"
  ]
}
EOF
                sudo systemctl restart docker
                ok "Docker 镜像源已配置"
            fi
        else
            ok "Docker daemon 已通过代理访问，跳过镜像源配置"
        fi
    fi

    build_image
    compose_up
    echo ""
    echo -e "${BOLD}部署完成！${NC}"
    echo "  查看状态: $_ME doctor"
    echo "  查看日志: $_ME logs"
    echo "  停  止: $_ME clean"
}

# ====== 构建 ======
build_image() {
    echo -e "${GREEN}[docker] 构建镜像 shortener-app:latest ...${NC}"
    _build_flags_info
    local build_cmd
    build_cmd=$(_detect_build_flags)
    echo "  命令: $build_cmd -t shortener-app:latest -f Dockerfile ."
    eval "$build_cmd -t shortener-app:latest -f Dockerfile ."
    ok "镜像构建完成"
}

# ====== 启动 ======
compose_up() {
    echo -e "${GREEN}[docker] docker compose up -d ...${NC}"
    _compose up -d
    ok "服务已启动"
    echo "  MySQL  → localhost:3307"
    echo "  Redis  → (内部)"
    echo "  App    → http://localhost:${APP_PORT:-8889}"
    _compose ps
}

logs() {
    _compose logs -f app
}

clean() {
    echo -e "${GREEN}[docker] 停止容器 + 删除镜像 ...${NC}"
    _compose down -v 2>/dev/null || true
    docker rmi shortener-app:latest 2>/dev/null || true
    ok "清理完成"
}

# ====== 主入口 ======
case "${1:-doctor}" in
    doctor)  doctor ;;
    setup)   setup ;;
    build)   build_image ;;
    compose) compose_up ;;
    logs)    logs ;;
    clean)   clean ;;
    *)
        echo "用法: $_ME [doctor|setup|build|compose|logs|clean]"
        echo ""
        echo "  首次部署: doctor  → 诊断环境（Docker/权限/网络/Compose）"
        echo "            setup   → 自动配网 + 构建 + 启动（推荐）"
        echo ""
        echo "  日常使用: build   → 构建镜像"
        echo "            compose → 启动全部服务（MySQL + Redis + App）"
        echo "            logs    → 查看 App 日志"
        echo "            clean   → 停止容器 + 删除镜像"
        ;;
esac
