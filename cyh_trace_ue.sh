#!/usr/bin/env bash
# 使用 uftrace 追踪 UE 的函数调用树
# 重点追踪：RRC、NAS 消息处理

set -e

echo "================================================"
echo "使用 uftrace 追踪 NR UE"
echo "================================================"
echo "⚠️  确保已经用 cyh_rebuild_for_uftrace.sh 重新编译！"
echo "⚠️  确保 gNB 已经在运行！"
echo ""
echo "📊 追踪范围："
echo "   - openair2/RRC (RRC 协议栈)"
echo "   - openair3/NAS (NAS 层)"
echo "   - common/utils/itti (消息任务)"
echo ""
echo "💡 提示："
echo "   - 等 UE 完成注册和 PDU Session 建立后，按 Ctrl+C 停止"
echo "   - trace 数据会保存到 /tmp/oai_ue_trace/"
echo ""
read -p "按 Enter 继续..."

cd ~/openairinterface5g/cmake_targets/ran_build/build

# 创建输出目录
TRACE_DIR="/tmp/oai_ue_trace_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$TRACE_DIR"

echo ""
echo "================================================"
echo "开始追踪 UE..."
echo "================================================"

sudo uftrace record \
  -d "$TRACE_DIR" \
  --srcline \
  --no-libcall \
  -D 12 \
  -t 5us \
  -L openair2/RRC \
  -L openair3/NAS \
  -L common/utils/itti \
  -- \
  ./nr-uesoftmodem \
    --rfsim --rfsimulator.serveraddr 127.0.0.1 \
    -r 106 --numerology 1 --band 78 -C 3619200000 --ssb 516 \
    -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf

echo ""
echo "================================================"
echo "✅ Trace 数据已保存到: $TRACE_DIR"
echo "================================================"
echo ""
echo "📊 查看调用树："
echo "   uftrace replay -d $TRACE_DIR --srcline --no-libcall -D 15 -t 5us | less"
echo ""
echo "🔍 只看 RRC Setup 流程："
echo "   uftrace replay -d $TRACE_DIR --srcline --no-libcall -D 20 -t 0us -F 'nr_rrc.*Setup.*' | less"
echo ""
echo "🔍 只看 NAS 认证流程："
echo "   uftrace replay -d $TRACE_DIR --srcline --no-libcall -D 20 -t 0us -F 'nas.*authentication.*' | less"
echo ""
echo "🎨 生成 Chrome Trace (时间线)："
echo "   uftrace dump -d $TRACE_DIR --chrome > $TRACE_DIR/ue_trace.json"
echo "   然后在 Chrome 浏览器中打开 chrome://tracing，加载这个 json 文件"
echo ""
