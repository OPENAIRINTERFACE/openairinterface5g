#!/bin/bash
# uftrace 使用示例 - 测试是否工作

echo "====== uftrace 功能测试 ======"
echo ""
echo "此脚本将测试 uftrace 是否能正确追踪 C 程序"
echo ""

# 创建测试程序
cat > /tmp/test_uftrace.c <<'EOF'
#include <stdio.h>
#include <unistd.h>

void func_c() {
    printf("  In func_c\n");
    usleep(10000); // 10ms
}

void func_b() {
    printf(" In func_b\n");
    func_c();
    usleep(5000); // 5ms
}

void func_a() {
    printf("In func_a\n");
    func_b();
    usleep(3000); // 3ms
}

int main() {
    printf("Starting test...\n");
    func_a();
    printf("Test complete!\n");
    return 0;
}
EOF

echo "步骤 1: 编译测试程序（需要 -pg 选项）"
gcc -pg -g -O0 -o /tmp/test_uftrace /tmp/test_uftrace.c
echo "  完成: /tmp/test_uftrace"
echo ""

echo "步骤 2: 使用 uftrace 追踪"
uftrace -d /tmp/uftrace_test /tmp/test_uftrace
echo ""

echo "步骤 3: 查看函数调用树"
echo "----------------------------------------"
uftrace replay -d /tmp/uftrace_test
echo "----------------------------------------"
echo ""

echo "步骤 4: 查看函数统计"
echo "----------------------------------------"
uftrace report -d /tmp/uftrace_test
echo "----------------------------------------"
echo ""

echo "====== 测试完成 ======"
echo ""
echo "如果您看到了类似这样的输出:"
echo "  # DURATION     TID     FUNCTION"
echo "            [  xxxx] | main() {"
echo "   xx.xxx ms [  xxxx] |   func_a() {"
echo "   xx.xxx ms [  xxxx] |     func_b() {"
echo "   xx.xxx ms [  xxxx] |       func_c();"
echo ""
echo "那么 uftrace 工作正常！"
echo ""
echo "现在可以用同样的方式追踪 OAI UE:"
echo "  ./cyh_quick_uftrace.sh <ue_config.conf>"
echo ""
echo "注意：OAI 程序需要使用 Debug 模式编译才能被 uftrace 追踪："
echo "  cd ~/openairinterface5g/cmake_targets"
echo "  ./build_oai -g Debug --nrUE -w SIMU"
