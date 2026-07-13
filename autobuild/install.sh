#!/bin/bash

# dlmuduo 网络库安装脚本
# 使用方法: sudo ./install.sh [安装路径]
# 默认安装路径: /usr/local
# 无需手动装依赖 — 脚本自动检测并安装

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
NC='\033[0m'

print_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
print_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
print_error(){ echo -e "${RED}[ERROR]${NC} $1"; }

# ====== 尝试安装系统依赖（无则自动安装） ======
# 用法: try_install_pkg "检测命令" "apt包名" "yum/dnf包名"
try_install_pkg() {
    local check="$1" pkg_apt="$2" pkg_yum="$3" desc="$4"
    if eval "$check" 2>/dev/null; then
        print_info "$desc 已就绪 ✓"
        return 0
    fi
    print_warn "$desc 未安装，尝试自动安装..."
    if command -v apt-get >/dev/null 2>&1; then
        apt-get update -qq 2>/dev/null || true
        if apt-get install -y $pkg_apt; then
            print_info "$desc 安装完成 ✓"
            return 0
        fi
    elif command -v dnf >/dev/null 2>&1; then
        if dnf install -y $pkg_yum; then
            print_info "$desc 安装完成 ✓"
            return 0
        fi
    elif command -v yum >/dev/null 2>&1; then
        if yum install -y $pkg_yum; then
            print_info "$desc 安装完成 ✓"
            return 0
        fi
    fi
    print_error "自动安装 $desc 失败。请手动安装："
    echo "  Ubuntu/Debian: sudo apt-get install $pkg_apt"
    echo "  CentOS/RHEL:   sudo yum install $pkg_yum"
    exit 1
}

# ====== 自动定位项目根目录 ======
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
    print_error "未找到项目根目录（CMakeLists.txt）"
    exit 1
fi
cd "$PROJECT_ROOT"

INSTALL_PREFIX="${1:-/usr/local}"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  dlmuduo 网络库安装脚本${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# ====== 检查 root 权限 ======
if [ "$EUID" -ne 0 ]; then
    if [ "$INSTALL_PREFIX" = "/usr/local" ]; then
        print_error "安装到系统路径需要 root 权限"
        echo "  sudo bash $0"
        echo "  或者安装到用户目录: bash $0 ~/local"
        exit 1
    fi
    print_warn "非 root 用户，依赖安装可能需要 sudo"
fi

# ====== 自动安装依赖（复用 autobuild/deps.sh） ======
echo -e "${BOLD}检查并安装系统依赖...${NC}"
bash "$SCRIPT_DIR/deps.sh" lib

echo ""

# ====== 构建安装 ======
print_info "[1/4] 清理旧的构建文件..."
rm -rf build
mkdir -p build

print_info "[2/4] 配置 CMake..."
cd build
cmake -DBUILD_APPS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" ..

print_info "[3/4] 编译网络库..."
cmake --build . --target lczmuduo -j$(nproc)

print_info "[4/4] 安装到系统..."
make install

cd ..

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  安装完成！${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "安装路径: ${YELLOW}$INSTALL_PREFIX${NC}"
echo -e "库文件:   ${YELLOW}$INSTALL_PREFIX/lib/liblczmuduo.a${NC}"
echo -e "头文件:   ${YELLOW}$INSTALL_PREFIX/include/lczmuduo/${NC}"
echo ""
echo -e "${GREEN}使用方法:${NC}"
echo ""
echo "  g++ your_code.cpp -llczmuduo -lpthread -o your_program"
echo ""
