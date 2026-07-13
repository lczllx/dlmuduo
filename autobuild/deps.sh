#!/bin/bash
set -e

# ============================================================
# deps.sh — 自动拉取并验证所有构建依赖
# 用法: bash autobuild/deps.sh         安装全部依赖
#       bash autobuild/deps.sh lib      只装网络库依赖
#       bash autobuild/deps.sh app      只装应用依赖
# ============================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

ok()   { echo -e "  ${GREEN}[✓]${NC} $1"; }
fail() { echo -e "  ${RED}[✗]${NC} $1"; }

MODE="${1:-all}"

# ====== sudo ======
# 恢复终端 echo（防止 sudo 中断导致终端不显示输入）
_restore_tty() { stty echo 2>/dev/null; }
trap '_restore_tty' EXIT

if [ "$EUID" -ne 0 ]; then
    SUDO="sudo"
    echo -e "${YELLOW}需要 root 权限来安装依赖，请输入密码${NC}"
    sudo -v || { echo -e "${RED}sudo 密码验证失败${NC}"; exit 1; }
else
    SUDO=""
fi

# ====== 包管理器 ======
_apt() {
    if command -v apt-get >/dev/null 2>&1; then
        $SUDO apt-get update -qq 2>/dev/null || true
        $SUDO apt-get install -y "$@"
    elif command -v dnf >/dev/null 2>&1; then
        $SUDO dnf install -y "$@"
    elif command -v yum >/dev/null 2>&1; then
        $SUDO yum install -y "$@"
    else
        echo -e "${RED}未检测到支持的包管理器${NC}"
        exit 1
    fi
}

# ====== 验证头文件存在 ======
_check_header() {
    local header="$1" pkg="$2"
    if echo "#include <$header>" | g++ -fsyntax-only -x c++ - 2>/dev/null; then
        ok "$header"
    else
        fail "$header → 安装 $pkg"
        _apt "$pkg"
        if echo "#include <$header>" | g++ -fsyntax-only -x c++ - 2>/dev/null; then
            ok "$header (已修复)"
        else
            fail "$header 仍然找不到，请手动处理"
            return 1
        fi
    fi
}

# ====== 验证库文件 ======
_check_lib() {
    local lib="$1" pkg="$2"
    if ldconfig -p 2>/dev/null | grep -q "$lib" || find /usr/lib -name "$lib*" 2>/dev/null | grep -q .; then
        ok "lib$lib"
    else
        fail "lib$lib → 安装 $pkg"
        _apt "$pkg"
        if ldconfig -p 2>/dev/null | grep -q "$lib" || find /usr/lib -name "$lib*" 2>/dev/null | grep -q .; then
            ok "lib$lib (已修复)"
        else
            fail "lib$lib 仍然找不到，请手动处理"
            return 1
        fi
    fi
}

# ====== 基础编译工具 ======
install_base() {
    echo -e "${GREEN}[基础工具]${NC}"
    if ! command -v cmake >/dev/null 2>&1; then
        fail "cmake → 安装中..."
        _apt cmake
    fi
    command -v cmake >/dev/null 2>&1 && ok "cmake ($(cmake --version | head -1 | awk '{print $NF}'))"

    if ! command -v g++ >/dev/null 2>&1; then
        fail "g++ → 安装中..."
        _apt g++
    fi
    command -v g++ >/dev/null 2>&1 && ok "g++ ($(g++ --version | head -1 | awk '{print $NF}'))"

    if ! command -v make >/dev/null 2>&1; then
        fail "make → 安装中..."
        _apt make
    fi
    command -v make >/dev/null 2>&1 && ok "make"
}

# ====== 应用依赖 ======
install_app() {
    echo -e "${GREEN}[应用依赖]${NC}"

    # pkg-config
    if ! command -v pkg-config >/dev/null 2>&1; then
        fail "pkg-config → 安装中..."
        _apt pkg-config
    fi
    command -v pkg-config >/dev/null 2>&1 && ok "pkg-config"

    # mysql/mysql.h（用 libmysqlclient-dev，头文件路径是 /usr/include/mysql/）
    _check_header "mysql/mysql.h" "libmysqlclient-dev"

    # hiredis/hiredis.h
    _check_header "hiredis/hiredis.h" "libhiredis-dev"

    # jsoncpp/json/json.h
    _check_header "jsoncpp/json/json.h" "libjsoncpp-dev"

    # 运行时库
    _check_lib "libmysqlclient" "libmysqlclient-dev"
    _check_lib "libhiredis" "libhiredis-dev"
    _check_lib "libjsoncpp" "libjsoncpp-dev"
}

echo -e "${GREEN}========================================${NC}"
case "$MODE" in
    lib)
        echo -e "${GREEN}  安装网络库依赖${NC}"
        echo -e "${GREEN}========================================${NC}"
        install_base
        ;;
    app)
        echo -e "${GREEN}  安装应用依赖${NC}"
        echo -e "${GREEN}========================================${NC}"
        install_app
        ;;
    all)
        echo -e "${GREEN}  安装全部构建依赖${NC}"
        echo -e "${GREEN}========================================${NC}"
        install_base
        echo ""
        install_app
        ;;
    *)
        echo "用法: bash autobuild/deps.sh [lib|app|all]"
        exit 1
        ;;
esac
echo ""
echo -e "${GREEN}依赖就绪，可以构建：${NC}"
echo "  mkdir -p build && cd build && cmake .. && make -j\$(nproc)"
