#!/usr/bin/env bash
# 快速追踪UE注册流程（简化版，30秒）

set -e

echo "================================================"
echo "快速UE注册流程追踪"
echo "================================================"
echo ""
echo "此脚本将："
echo "1. 编译插桩版本的gNB和UE（如果需要）"
echo "2. 启动gNB并追踪30秒"
echo "3. 等待UE连接"
echo "4. 生成注册流程分析报告"
echo ""

# 检查是否已经编译了插桩版本
if [ ! -f ~/openairinterface5g/cmake_targets/ran_build/build/nr-softmodem ]; then
    echo "⚠️  未找到编译的程序，正在编译插桩版本..."
    ~/openairinterface5g/cyh_rebuild_with_instrument.sh
else
    echo "✅ 找到已编译的程序"
fi

# 创建trace目录
TRACE_DIR="/tmp/oai_ue_registration_trace_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$TRACE_DIR"

echo ""
echo "================================================"
echo "启动gNB并开始追踪..."
echo "================================================"
echo "Trace目录: $TRACE_DIR"
echo "追踪时长: 30秒"
echo ""
echo "⚠️  请在另一个终端启动UE："
echo "    cd ~/openairinterface5g"
echo "    ./cyh_start_ue.sh"
echo ""
echo "按Enter开始追踪..."
read

cd ~/openairinterface5g/cmake_targets/ran_build/build

# 设置库路径
export LD_LIBRARY_PATH=$(pwd):/usr/lib/x86_64-linux-gnu/uftrace:$LD_LIBRARY_PATH

# 启动gNB并追踪30秒
timeout 30s sudo -E uftrace record \
    -d "$TRACE_DIR" \
    --no-libcall \
    -D 15 \
    -t 5us \
    -N 'std::*' -N '__cxa*' -N '_IO*' -N 'operator*' \
    ./nr-softmodem \
    -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf \
    --gNBs.[0].min_rxtxtime 6 \
    --rfsim || echo "追踪已完成（30秒超时）"

echo ""
echo "================================================"
echo "追踪完成！开始分析..."
echo "================================================"

# 调用分析脚本
~/openairinterface5g/cyh_analyze_ue_registration_flow.sh "$TRACE_DIR"

echo ""
echo "================================================"
echo "✅ 完成！"
echo "================================================"
echo ""
echo "Trace位置: $TRACE_DIR"
echo ""
echo "查看分析结果："
echo "  cat ${TRACE_DIR}/registration_analysis/*.txt"
echo ""
