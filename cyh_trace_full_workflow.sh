#!/usr/bin/env bash
# 完整的 gNB+UE 追踪工作流
# 自动启动 gNB 追踪，等待您手动启动 UE

set -e

echo "================================================"
echo "OAI gNB + UE 函数调用追踪工作流"
echo "================================================"
echo ""
echo "🎯 这个脚本会："
echo "   1. 启动 gNB（带 uftrace 追踪）"
echo "   2. 等待您在另一个终端启动 UE"
echo "   3. 追踪 60 秒后自动停止"
echo "   4. 生成调用树报告"
echo ""
echo "⚠️  前置条件："
echo "   - L25GC+ 核心网已启动"
echo "   - 已用 cyh_rebuild_with_instrument.sh 重新编译"
echo ""
read -p "按 Enter 继续..."

cd ~/openairinterface5g/cmake_targets/ran_build/build

# 创建输出目录
TRACE_DIR="/tmp/oai_gnb_trace_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$TRACE_DIR"

echo ""
echo "================================================"
echo "启动 gNB（带 uftrace 追踪，60秒后自动停止）"
echo "================================================"
echo "📍 Trace 目录: $TRACE_DIR"
echo ""
echo "💡 现在请在另一个终端运行 UE："
echo "   cd ~/openairinterface5g/cmake_targets/ran_build/build"
echo "   export LD_LIBRARY_PATH=\$(pwd):\$LD_LIBRARY_PATH"
echo "   sudo -E ./nr-uesoftmodem --rfsim --rfsimulator.serveraddr 127.0.0.1 \\"
echo "     -r 106 --numerology 1 --band 78 -C 3619200000 --ssb 516 \\"
echo "     -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf"
echo ""
sleep 3

# 使用 bash -c 包装，避免 sudo 丢失环境变量
sudo bash -c "
export LD_LIBRARY_PATH=$(pwd):/usr/lib/x86_64-linux-gnu/uftrace:\$LD_LIBRARY_PATH
timeout 120s uftrace record \
  -d '$TRACE_DIR' \
  --no-libcall \
  -D 15 \
  -t 5us \
  ./nr-softmodem \
    -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf \
    --gNBs.[0].min_rxtxtime 6 \
    --rfsim
" || true

echo ""
echo "================================================"
echo "✅ 追踪完成！"
echo "================================================"
echo "📍 Trace 数据保存在: $TRACE_DIR"
echo ""
echo "📊 正在生成调用树（前 1000 行）..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 15 -t 5us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | head -1000 > "$TRACE_DIR/call_tree.txt"
echo "✅ 已保存到: $TRACE_DIR/call_tree.txt"
echo ""
echo "📈 正在生成函数统计..."
uftrace report -d "$TRACE_DIR" --no-libcall -s total -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | head -100 > "$TRACE_DIR/function_stats.txt"
echo "✅ 已保存到: $TRACE_DIR/function_stats.txt"
echo ""
echo "🔍 查找 RRC 相关函数..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 20 -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | grep -iE 'rrc|RRC' | head -500 > "$TRACE_DIR/rrc_calls.txt" || true
echo "✅ 已保存到: $TRACE_DIR/rrc_calls.txt"
echo ""
echo "🔍 查找 NGAP 相关函数..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 20 -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | grep -iE 'ngap|NGAP' | head -500 > "$TRACE_DIR/ngap_calls.txt" || true
echo "✅ 已保存到: $TRACE_DIR/ngap_calls.txt"
echo ""
echo "================================================"
echo "📁 查看结果："
echo "================================================"
echo "  完整调用树:  cat $TRACE_DIR/call_tree.txt | less"
echo "  函数统计:    cat $TRACE_DIR/function_stats.txt | less"
echo "  RRC 调用:    cat $TRACE_DIR/rrc_calls.txt | less"
echo "  NGAP 调用:   cat $TRACE_DIR/ngap_calls.txt | less"
echo ""
echo "🎨 生成可视化:"
echo "  Chrome Trace: uftrace dump -d $TRACE_DIR --chrome > $TRACE_DIR/trace.json"
echo "  GraphViz:     uftrace dump -d $TRACE_DIR --graphviz > $TRACE_DIR/callgraph.dot"
echo ""
