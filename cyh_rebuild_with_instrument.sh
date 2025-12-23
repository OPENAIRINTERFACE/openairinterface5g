#!/usr/bin/env bash
# 正确编译支持 uftrace 的 OAI（通过环境变量设置编译标志）

set -e

cd ~/openairinterface5g
source oaienv
cd cmake_targets

echo "================================================"
echo "清理旧的编译文件"
echo "================================================"
rm -rf ran_build/build

echo ""
echo "================================================"
echo "编译 OAI with -finstrument-functions"
echo "================================================"
echo "这会花 5-10 分钟..."
echo ""

# 关键：通过环境变量设置编译标志
# -finstrument-functions 会在每个函数进入/退出时调用钩子
# -O0 完全不优化，避免函数被 inline
export CFLAGS="-g -O0 -finstrument-functions -fno-omit-frame-pointer -fno-inline"
export CXXFLAGS="-g -O0 -finstrument-functions -fno-omit-frame-pointer -fno-inline"

# 打印确认
echo "CFLAGS=$CFLAGS"
echo "CXXFLAGS=$CXXFLAGS"
echo ""

./build_oai --gNB --nrUE -w SIMU -g Debug

echo ""
echo "================================================"
echo "验证编译结果..."
echo "================================================"

# 检查是否包含插桩符号
if nm ran_build/build/nr-softmodem | grep -q 'cyg_profile'; then
    echo "✅ 成功！nr-softmodem 包含函数插桩"
else
    echo "⚠️  警告：未检测到插桩符号，uftrace 可能需要用 -P 动态追踪"
fi

echo ""
echo "================================================"
echo "✅ 编译完成！"
echo "================================================"
echo ""
echo "现在可以运行："
echo "  ./cyh_trace_gnb_instrumented.sh  # 使用插桩版本追踪"
echo "  或"
echo "  ./cyh_trace_gnb_dynamic.sh      # 使用动态追踪"
echo ""
