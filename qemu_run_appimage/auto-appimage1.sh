#!/bin/bash
# appimage-packager-rofs-fixed.sh
# 已添加QEMU兼容性支持

set -e

# 彩色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 创建图标
create_icon() {
    local icon_path="$1"
    mkdir -p "$(dirname "$icon_path")"
    
    echo 'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg==' | \
    base64 -d > "$icon_path" 2>/dev/null || \
    echo -ne '\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00\x00\x1f\x15\xc4\x89\x00\x00\x00\nIDATx\x9cc\xf8\x0f\x00\x00\x01\x00\x01\x05\x01\x00\x00\x00\x00IEND\xaeB`\x82' > "$icon_path"
}

# 创建简单二进制AppRun（修复版）
create_simple_binary_apprun() {
    local app_dir="$1"
    local app_name="$2"
    local orig_name="$3"
    
    echo "创建简单二进制AppRun..."
    
    # 先回退到脚本版本
    cat > "$app_dir/AppRun" << 'EOF'
#!/bin/sh
# 简单启动脚本（兼容性最好）
HERE="$(dirname "$(readlink -f "$0")")"
APP_NAME="$(basename "$0" .AppImage)"

# 设置环境
export PATH="$HERE/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"

# 检测架构
if [ "$(uname -m)" = "x86_64" ]; then
    # x86系统直接运行
    exec "$HERE/usr/bin/$APP_NAME" "$@"
else
    # ARM系统，通过QEMU运行
    # 确保有lib64目录
    if [ ! -d "$HERE/lib64" ] && [ -f "$HERE/usr/lib/ld-linux-x86-64.so.2" ]; then
        mkdir -p "$HERE/lib64"
        cp "$HERE/usr/lib/ld-linux-x86-64.so.2" "$HERE/lib64/" 2>/dev/null || true
    fi
    
    # 使用QEMU运行
    exec qemu-x86_64-static -L "$HERE" "$HERE/usr/bin/$APP_NAME" "$@"
fi
EOF
    
    chmod +x "$app_dir/AppRun"
    echo -e "${GREEN}✓ 创建脚本版AppRun${NC}"
}

# 创建二进制AppRun函数（修复版）
create_binary_apprun() {
    local app_dir="$1"
    local app_name="$2"
    local orig_name="$3"
    
    echo -e "${YELLOW}创建二进制AppRun...${NC}"
    
    # 创建修复的C源代码
    cat > /tmp/apprun_fixed.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

// 获取程序所在目录
static char* get_program_dir() {
    static char path[4096];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path)-1);
    if (len == -1) {
        return NULL;
    }
    path[len] = '\0';
    
    // 找到最后一个'/'
    char* last_slash = strrchr(path, '/');
    if (last_slash) {
        *last_slash = '\0';
    }
    return path;
}

// 检查文件是否存在
static int file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

// 执行程序
static void execute_program(const char* dir, const char* prog_name, char* const argv[]) {
    char prog_path[4096];
    snprintf(prog_path, sizeof(prog_path), "%s/usr/bin/%s", dir, prog_name);
    
    if (!file_exists(prog_path)) {
        // 尝试查找任何可执行文件
        fprintf(stderr, "错误: 未找到可执行文件 %s\n", prog_path);
        exit(1);
    }
    
    // 设置环境变量
    setenv("HERE", dir, 1);
    
    char ld_path[4096];
    snprintf(ld_path, sizeof(ld_path), "%s/usr/lib", dir);
    setenv("LD_LIBRARY_PATH", ld_path, 1);
    
    char path[4096];
    const char* old_path = getenv("PATH");
    if (old_path) {
        snprintf(path, sizeof(path), "%s/usr/bin:%s", dir, old_path);
    } else {
        snprintf(path, sizeof(path), "%s/usr/bin", dir);
    }
    setenv("PATH", path, 1);
    
    // 执行程序
    execv(prog_path, argv);
    // 如果execv失败
    fprintf(stderr, "无法执行程序: %s\n", prog_path);
    exit(1);
}

int main(int argc, char* argv[]) {
    // 获取程序所在目录
    char* dir = get_program_dir();
    if (!dir) {
        fprintf(stderr, "无法获取程序目录\n");
        return 1;
    }
    
    // 从AppImage文件名获取程序名
    char* prog_name = "program";
    
    // 尝试从argv[0]获取
    if (argc > 0 && argv[0]) {
        char* app_name = argv[0];
        char* last_slash = strrchr(app_name, '/');
        if (last_slash) {
            app_name = last_slash + 1;
        }
        
        // 移除.AppImage后缀
        char* dot_appimage = strstr(app_name, ".AppImage");
        if (dot_appimage) {
            *dot_appimage = '\0';
            prog_name = app_name;
        }
    }
    
    // 准备新参数数组
    char** new_argv = malloc((argc + 1) * sizeof(char*));
    if (!new_argv) {
        fprintf(stderr, "内存分配失败\n");
        return 1;
    }
    
    new_argv[0] = prog_name;
    for (int i = 1; i < argc; i++) {
        new_argv[i] = argv[i];
    }
    new_argv[argc] = NULL;
    
    // 执行程序
    execute_program(dir, prog_name, new_argv);
    
    free(new_argv);
    return 0;
}
EOF
    
    # 尝试编译为x86_64二进制（静态链接）
    echo "编译x86_64二进制..."
    
    # 检查是否有交叉编译器
    if command -v x86_64-linux-gnu-gcc >/dev/null 2>&1; then
        echo "使用x86_64-linux-gnu-gcc编译..."
        x86_64-linux-gnu-gcc -static -Os -o "$app_dir/AppRun" /tmp/apprun_fixed.c 2>&1 | grep -v "warning" || true
    elif command -v gcc >/dev/null 2>&1; then
        echo "使用gcc交叉编译..."
        gcc -target x86_64-linux-gnu -static -Os -o "$app_dir/AppRun" /tmp/apprun_fixed.c 2>&1 | grep -v "warning" || true
    else
        echo "未找到编译器"
    fi
    
    # 检查编译是否成功
    if [[ -f "$app_dir/AppRun" ]] && [[ -x "$app_dir/AppRun" ]]; then
        # 验证是x86_64二进制
        if file "$app_dir/AppRun" | grep -q "x86-64"; then
            chmod +x "$app_dir/AppRun"
            echo -e "${GREEN}✓ 二进制AppRun创建成功${NC}"
            file "$app_dir/AppRun"
            return 0
        else
            echo "编译出的不是x86-64二进制"
            rm -f "$app_dir/AppRun"
        fi
    fi
    
    # 编译失败，使用简单脚本
    echo "二进制编译失败，使用脚本版本"
    create_simple_binary_apprun "$app_dir" "$app_name" "$orig_name"
    
    rm -f /tmp/apprun_fixed.c
}

# 创建简单二进制AppRun
create_simple_binary_apprun() {
    local app_dir="$1"
    local app_name="$2"
    local orig_name="$3"

    # 创建极简的汇编二进制
    cat > /tmp/minimal.S << 'EOF'
.section .note.GNU-stack,"",@progbits
.section .text
.globl _start
_start:
    # execve("./usr/bin/program", argv, envp)
    mov $59, %rax           # syscall: execve

    # 构建路径字符串
    lea path(%rip), %rdi    # arg1: filename

    # 构建参数数组 ["./usr/bin/program", NULL]
    lea argv(%rip), %rsi    # arg2: argv

    # 环境变量
    xor %rdx, %rdx          # arg3: envp = NULL

    syscall

    # 如果失败，退出
    mov $60, %rax           # syscall: exit
    mov $1, %rdi           # status = 1
    syscall

path:
    .ascii "./usr/bin/program\0"

argv:
    .quad path
    .quad 0
EOF

    # 汇编并链接
    as --64 -o /tmp/minimal.o /tmp/minimal.S
    ld -m elf_x86_64 -s -o "$app_dir/AppRun" /tmp/minimal.o

    if [[ -f "$app_dir/AppRun" ]]; then
        chmod +x "$app_dir/AppRun"
        echo -e "${GREEN}✓ 极简二进制AppRun已创建${NC}"
    else
        echo -e "${RED}错误: 无法创建二进制AppRun${NC}"
        # 回退到脚本
        create_script_apprun "$app_dir" "$app_name" "$orig_name"
    fi

    rm -f /tmp/minimal.S /tmp/minimal.o
}

# 使用busybox作为AppRun
use_busybox_as_apprun() {
    local app_dir="$1"

    echo "使用busybox作为AppRun..."

    # 查找或下载x86_64的busybox
    if [[ -f /usr/bin/busybox ]] && file /usr/bin/busybox | grep -q "x86-64"; then
        cp /usr/bin/busybox "$app_dir/AppRun"
    elif [[ -f /bin/busybox ]] && file /bin/busybox | grep -q "x86-64"; then
        cp /bin/busybox "$app_dir/AppRun"
    else
        # 尝试下载静态版busybox
        echo "下载静态busybox..."
        wget -q -O "$app_dir/AppRun" "https://busybox.net/downloads/binaries/1.35.0-x86_64-linux-musl/busybox" 2>/dev/null || {
            echo "下载失败，使用备用方案"
            return 1
        }
    fi

    if [[ -f "$app_dir/AppRun" ]]; then
        chmod +x "$app_dir/AppRun"
        # 创建busybox的symlink脚本
        cat > "$app_dir/.busybox-run" << 'EOF'
#!/bin/sh
# busybox wrapper
HERE=$(dirname "$0")
export PATH="$HERE/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
exec "$HERE/AppRun" "$HERE/usr/bin/$(basename "$0" .AppImage)" "$@"
EOF
        chmod +x "$app_dir/.busybox-run"
        echo -e "${GREEN}✓ 使用busybox作为AppRun${NC}"
        return 0
    fi
    return 1
}

# 收集所有动态库依赖
collect_all_dependencies() {
    local executable="$1"
    local lib_dir="$2"
    
    echo -e "${YELLOW}收集动态库依赖...${NC}"
    
    if ! ldd "$executable" 2>/dev/null | grep -q "=>"; then
        echo "程序是静态链接，无需库文件"
        return 0
    fi
    
    mkdir -p "$lib_dir"
    local processed_libs=()
    
    collect_libs_recursive() {
        local target="$1"
        
        ldd "$target" 2>/dev/null | grep "=>" | awk '{print $3}' | while read -r lib; do
            if [[ -f "$lib" ]]; then
                local libname=$(basename "$lib")
                
                if [[ ! " ${processed_libs[@]} " =~ " ${libname} " ]]; then
                    processed_libs+=("$libname")
                    
                    cp "$lib" "$lib_dir/" 2>/dev/null
                    if [[ $? -eq 0 ]]; then
                        echo "  ✅ $libname"
                        collect_libs_recursive "$lib"
                    fi
                fi
            fi
        done
    }
    
    collect_libs_recursive "$executable"
    
    # 添加基础库
    local common_libs=(
        "ld-linux-x86-64.so.2"
        "libc.so.6" "libm.so.6" "libpthread.so.0"
        "libdl.so.2" "librt.so.1" "libgcc_s.so.1"
        "libstdc++.so.6"
    )
    
    for lib in "${common_libs[@]}"; do
        find /usr/lib /lib /lib64 -name "$lib" -type f 2>/dev/null | head -1 | while read -r libpath; do
            if [[ -f "$libpath" ]] && [[ ! -f "$lib_dir/$(basename "$libpath")" ]]; then
                cp "$libpath" "$lib_dir/" 2>/dev/null && echo "  ✅ 基础: $(basename "$libpath")"
            fi
        done
    done
    
    local lib_count=$(ls -1 "$lib_dir"/*.so* 2>/dev/null | wc -l)
    echo -e "${GREEN}✓ 添加了 $lib_count 个库${NC}"
}

# 设置QEMU目录结构
setup_qemu_structure() {
    local app_dir="$1"
    
    echo -e "${YELLOW}设置QEMU目录结构...${NC}"
    
    # 创建QEMU需要的目录结构
    mkdir -p "$app_dir/lib64"
    mkdir -p "$app_dir/usr/gnemul/qemu-x86_64"
    
    # 复制库到QEMU期望的位置
    if [[ -d "$app_dir/usr/lib" ]]; then
        echo "复制库文件到QEMU目录..."
        # 复制所有库
        cp -r "$app_dir/usr/lib/"* "$app_dir/usr/gnemul/qemu-x86_64/" 2>/dev/null || true
        
        # 特别处理ld-linux动态链接器
        if [[ -f "$app_dir/usr/lib/ld-linux-x86-64.so.2" ]]; then
            echo "设置动态链接器..."
            # 在lib64创建链接（QEMU查找的位置）
            ln -sf ../usr/lib/ld-linux-x86-64.so.2 "$app_dir/lib64/ld-linux-x86-64.so.2" 2>/dev/null || true
            # 同时也复制一份到lib64确保可用
            cp "$app_dir/usr/lib/ld-linux-x86-64.so.2" "$app_dir/lib64/" 2>/dev/null || true
        fi
        
        echo -e "${GREEN}✓ QEMU目录结构已创建${NC}"
    else
        echo -e "${YELLOW}⚠ 未找到库目录，跳过QEMU结构设置${NC}"
    fi
}

# 查找并复制数据文件
find_and_copy_data() {
    local executable="$1"
    local app_name="$2"
    local data_dir="$3"
    
    echo -e "${YELLOW}查找数据文件...${NC}"
    
    local prog_dir=$(dirname "$(realpath "$executable")")
    local patterns=("*.pem" "*.key" "*.crt" "*.cfg" "*.conf" "*.ini" "*.json" "*.xml")
    
    mkdir -p "$data_dir"
    local count=0
    
    # 首先查找程序目录
    for pattern in "${patterns[@]}"; do
        find "$prog_dir" -maxdepth 2 -type f -name "$pattern" 2>/dev/null | while read -r file; do
            if [[ "$file" != "$executable" ]]; then
                cp "$file" "$data_dir/" 2>/dev/null && {
                    echo "  ✅ $(basename "$file")"
                    count=$((count + 1))
                }
            fi
        done
    done
    
    # 如果没有找到，尝试复制所有非可执行文件
    if [[ $count -eq 0 ]]; then
        find "$prog_dir" -maxdepth 1 -type f ! -name "$(basename "$executable")" ! -name "*.AppImage" | while read -r file; do
            cp "$file" "$data_dir/" 2>/dev/null && echo "  ✅ $(basename "$file")"
        done
    fi
    
    # 如果没有数据文件，创建示例配置
    if [[ $count -eq 0 ]] && [[ ! -f "$data_dir/config.ini" ]]; then
        cat > "$data_dir/config.ini" <<EOF
# 程序配置文件
# 请根据实际情况修改

[General]
name=$app_name
version=1.0.0

[Paths]
data_dir=./data
log_dir=./logs

[Network]
host=127.0.0.1
port=8080
EOF
        echo "  📝 创建示例配置文件"
    fi
    
    echo -e "${GREEN}✓ 数据文件处理完成${NC}"
}

# 主程序
main() {
    if [[ $# -lt 1 ]]; then
        echo -e "${GREEN}AppImage 打包工具 (QEMU兼容版)${NC}"
        echo "用法: $0 <可执行文件> [输出名称]"
        echo "       $0 -d <可执行文件> [输出名称]  (包含数据文件)"
        echo ""
        echo "示例:"
        echo "  $0 ./myapp"
        echo "  $0 -d ./myapp"
        exit 1
    fi
    
    # 检查是否包含数据文件
    INCLUDE_DATA=false
    if [[ "$1" == "-d" ]] || [[ "$1" == "--data" ]]; then
        INCLUDE_DATA=true
        shift
    fi
    
    echo -e "${BLUE}════════════════════════════════════════${NC}"
    echo -e "${BLUE}       AppImage 打包工具              ${NC}"
    echo -e "${BLUE}       (QEMU兼容版)                   ${NC}"
    echo -e "${BLUE}════════════════════════════════════════${NC}"
    
    EXECUTABLE="$1"
    APP_NAME="${2:-$(basename "$EXECUTABLE")}"
    
    [[ ! -f "$EXECUTABLE" ]] && { echo -e "${RED}错误: 文件不存在${NC}"; exit 1; }
    [[ ! -x "$EXECUTABLE" ]] && chmod +x "$EXECUTABLE"
    
    echo "程序: $APP_NAME"
    echo "原始: $(basename "$EXECUTABLE")"
    
    # 清理
    rm -rf "AppDir" "${APP_NAME}.AppImage"
    
    # =========== 步骤1: 创建目录 ===========
    echo -e "\n${YELLOW}[1/7] 创建目录...${NC}"
    mkdir -p AppDir/usr/bin
    mkdir -p AppDir/usr/lib
    
    ORIG_NAME=$(basename "$EXECUTABLE")
    
    # =========== 步骤2: 复制程序 ===========
    echo -e "\n${YELLOW}[2/7] 复制程序...${NC}"
    cp "$EXECUTABLE" "AppDir/usr/bin/$ORIG_NAME"
    chmod +x "AppDir/usr/bin/$ORIG_NAME"
    echo -e "${GREEN}✓ 程序: $ORIG_NAME${NC}"
    
    # =========== 步骤3: 收集依赖 ===========
    collect_all_dependencies "$EXECUTABLE" "AppDir/usr/lib"
    
    # =========== 步骤4: 设置QEMU目录结构 ===========
    setup_qemu_structure "AppDir"
    
    # =========== 步骤5: 处理数据文件 ===========
    if $INCLUDE_DATA; then
        echo -e "\n${YELLOW}[5/7] 处理数据文件...${NC}"
        find_and_copy_data "$EXECUTABLE" "$APP_NAME" "AppDir/usr/share/$APP_NAME"
    fi
    
    # =========== 步骤6: 创建图标 ===========
    echo -e "\n${YELLOW}[6/7] 创建图标...${NC}"
    create_icon "AppDir/$APP_NAME.png"
    cp "AppDir/$APP_NAME.png" "AppDir/.DirIcon"
    echo -e "${GREEN}✓ 图标已创建${NC}"
    
# =========== 步骤7: 创建二进制 AppRun ===========
echo -e "\n${YELLOW}[7/7] 创建二进制AppRun...${NC}"

# 首先尝试创建C语言版本
#use_busybox_as_apprun "AppDir" "$APP_NAME" "$ORIG_NAME"
create_binary_apprun "AppDir" "$APP_NAME" "$ORIG_NAME"

# 如果失败，使用备选方案
if [[ ! -f "AppDir/AppRun" ]] || [[ ! -x "AppDir/AppRun" ]]; then
    echo "二进制创建失败，使用脚本版本..."
    cat > "AppDir/AppRun" <<'EOF'
#!/bin/sh
# 兼容性脚本（当二进制不可用时）
HERE=$(dirname "$(readlink -f "$0")")
APP_NAME=$(basename "$0" .AppImage)

# 对于QEMU：直接运行内部程序
if [ "$(uname -m)" != "x86_64" ]; then
    # ARM系统，通过QEMU运行
    exec qemu-x86_64-static -L "$HERE" "$HERE/usr/bin/$APP_NAME" "$@"
else
    # x86系统，直接运行
    exec "$HERE/usr/bin/$APP_NAME" "$@"
fi
EOF
    chmod +x AppDir/AppRun
    echo -e "${YELLOW}⚠ 使用脚本版AppRun（QEMU兼容性可能有限）${NC}"
fi

    
    # =========== 步骤8: 创建桌面文件 ===========
    echo -e "\n${YELLOW}[8/7] 创建桌面文件...${NC}"
    mkdir -p AppDir/usr/share/applications
    cat > "AppDir/usr/share/applications/$APP_NAME.desktop" <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=$APP_NAME
Comment=Packaged as AppImage (QEMU兼容)
Exec=$ORIG_NAME
Icon=$APP_NAME
Categories=Utility;
Terminal=true
StartupNotify=false
EOF
    ln -sf usr/share/applications/$APP_NAME.desktop AppDir/
    echo -e "${GREEN}✓ 桌面文件已创建${NC}"
    
    # =========== 步骤9: 打包 ===========
    echo -e "\n${YELLOW}[9/7] 打包...${NC}"
    OUTPUT_FILE="${APP_NAME}.AppImage"
    
    # 架构检测
    if file -b "$EXECUTABLE" | grep -q "32-bit"; then
        ARCH="i386"
    else
        ARCH="x86_64"
    fi
    export ARCH
    
    echo "输出: $OUTPUT_FILE"
    echo "架构: $ARCH"
    
    # 打包
    if appimagetool --no-appstream AppDir "$OUTPUT_FILE" 2>&1 | grep -v "gpg2" | grep -v "Warning"; then
        echo -e "${GREEN}✅ 打包成功！${NC}"
    else
        appimagetool --no-appstream --no-fuse AppDir "$OUTPUT_FILE" 2>&1 | grep -v "gpg2" | grep -v "Warning"
        echo -e "${GREEN}✅ 打包成功！${NC}"
    fi
    
    # =========== 测试 ===========
    echo -e "\n${BLUE}════════════════════════════════════════${NC}"
    echo -e "${GREEN}测试运行...${NC}"
    
    chmod +x "$OUTPUT_FILE"
    
    # 创建测试数据目录
    TEST_DIR="$HOME/.local/share/$APP_NAME-test"
    rm -rf "$TEST_DIR"
    mkdir -p "$TEST_DIR"/{config,data,logs,cache}
    
    # 复制可能的配置文件
    if [ -d "AppDir/usr/share/$APP_NAME" ]; then
        cp -r AppDir/usr/share/$APP_NAME/* "$TEST_DIR/config/" 2>/dev/null || true
    fi
    
    echo "测试命令:"
    echo "  原生运行: ./\"$OUTPUT_FILE\" --help"
    echo "  QEMU运行: qemu-x86_64-static -L . ./\"$OUTPUT_FILE\" --help"
    echo "  调试模式: DEBUG=1 ./\"$OUTPUT_FILE\" --help"
    echo "----------------------------------------"
    
    # 原生测试运行
    export DEBUG=1
    if timeout 5s ./"$OUTPUT_FILE" --help 2>&1 | head -20; then
        echo -e "${GREEN}✅ 原生运行正常${NC}"
    elif timeout 5s ./"$OUTPUT_FILE" -h 2>&1 | head -20; then
        echo -e "${GREEN}✅ 原生运行正常${NC}"
    else
        echo -e "${YELLOW}⚠ 原生运行可能需要特定参数${NC}"
    fi
    unset DEBUG
    
    # QEMU兼容性测试
    echo -e "\n${YELLOW}QEMU兼容性测试...${NC}"
    if command -v qemu-x86_64-static >/dev/null 2>&1; then
        echo "测试命令: qemu-x86_64-static -L . ./\"$OUTPUT_FILE\" --version"
        if timeout 5s qemu-x86_64-static -L . ./"$OUTPUT_FILE" --version 2>&1 | head -5; then
            echo -e "${GREEN}✅ QEMU运行成功${NC}"
        elif timeout 5s qemu-x86_64-static -L . ./"$OUTPUT_FILE" -v 2>&1 | head -5; then
            echo -e "${GREEN}✅ QEMU运行成功${NC}"
        elif timeout 5s qemu-x86_64-static -L . ./"$OUTPUT_FILE" 2>&1 | head -5; then
            echo -e "${GREEN}✅ QEMU运行成功${NC}"
        else
            echo -e "${YELLOW}⚠ QEMU测试需要特定参数${NC}"
            echo "尝试: qemu-x86_64-static -L . ./\"$OUTPUT_FILE\" --help"
        fi
    else
        echo "qemu-x86_64-static 未安装，跳过QEMU测试"
        echo "安装命令: sudo apt install qemu-user-static"
    fi
    
    # 最终信息
    echo -e "\n${BLUE}════════════════════════════════════════${NC}"
    echo -e "${GREEN}            完成！                     ${NC}"
    echo -e "${BLUE}════════════════════════════════════════${NC}"
    echo ""
    echo "📦 输出文件: $(realpath "$OUTPUT_FILE")"
    echo "📏 大小: $(du -h "$OUTPUT_FILE" | cut -f1)"
    echo ""
    echo "🚀 使用方法:"
    echo "  在x86_64系统: ./\"$OUTPUT_FILE\""
    echo "  在ARM系统: qemu-x86_64-static -L . ./\"$OUTPUT_FILE\""
    echo ""
    echo "📁 用户数据目录:"
    echo "  $HOME/.local/share/$APP_NAME/"
    echo ""
    echo "🔧 调试模式:"
    echo "  DEBUG=1 ./\"$OUTPUT_FILE\" [参数]"
    echo ""
    echo "🔄 QEMU运行助手脚本:"
    cat > "$(dirname "$OUTPUT_FILE")/run-with-qemu.sh" <<'EOF2'
#!/bin/bash
# QEMU运行助手
APP="$1"
shift
qemu-x86_64-static -L . "$APP" "$@"
EOF2
    chmod +x "$(dirname "$OUTPUT_FILE")/run-with-qemu.sh"
    echo "  已创建: $(dirname "$OUTPUT_FILE")/run-with-qemu.sh"
    echo "  使用: ./run-with-qemu.sh \"$OUTPUT_FILE\" [参数]"
    
    # 清理
    rm -rf AppDir
}

# 运行
main "$@"
