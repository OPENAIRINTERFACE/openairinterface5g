#!/usr/bin/env bash
# 使用 uftrace 追踪 gNB 的函数调用树
# 重点追踪：RRC、NGAP、ITTI 消息处理

set -e

echo "================================================"
echo "使用 uftrace 追踪 gNB"
echo "================================================"
echo "⚠️  确保已经用 cyh_rebuild_for_uftrace.sh 重新编译！"
echo ""
echo "📊 追踪范围："
echo "   - openair2/RRC (RRC 协议栈)"
echo "   - openair3/NGAP (核心网接口)"
echo "   - common/utils/itti (消息任务)"
echo ""
echo "💡 提示："
echo "   - 等 UE 完成注册和 PDU Session 建立后，按 Ctrl+C 停止"
echo "   - trace 数据会保存到 /tmp/oai_gnb_trace/"
echo ""
read -p "按 Enter 继续..."

cd ~/openairinterface5g/cmake_targets/ran_build/build

# 创建输出目录
TRACE_DIR="/tmp/oai_gnb_trace_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$TRACE_DIR"

echo ""
echo "================================================"
echo "开始追踪 gNB..."
echo "================================================"

sudo uftrace record \
  -d "$TRACE_DIR" \
  --auto-args \
  --no-libcall \
  -D 12 \
  -t 5us \
  -N 'std::*' \
  -N '__*' \
  -- \
  ./nr-softmodem \
    -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf \
    --gNBs.[0].min_rxtxtime 6 \
    --rfsim

echo ""
echo "================================================"
echo "✅ Trace 数据已保存到: $TRACE_DIR"
echo "================================================"
echo ""
echo "📊 查看调用树："
echo "   uftrace replay -d $TRACE_DIR --srcline --no-libcall -D 15 -t 5us | less"
echo ""
echo "🎨 生成 Chrome Trace (时间线)："
echo "   uftrace dump -d $TRACE_DIR --chrome > $TRACE_DIR/gnb_trace.json"
echo "   然后在 Chrome 浏览器中打开 chrome://tracing，加载这个 json 文件"
echo ""
echo "📈 生成调用图："
echo "   uftrace dump -d $TRACE_DIR --graphviz > $TRACE_DIR/gnb_callgraph.dot"
echo "   dot -Tpng $TRACE_DIR/gnb_callgraph.dot -o $TRACE_DIR/gnb_callgraph.png"
echo ""
