#!/usr/bin/env bash
# 使用 uftrace 追踪 gNB 的函数调用树
# 修复版：处理 libmcount.so 路径问题

set -e

echo "================================================"
echo "使用 uftrace 追踪 gNB"
echo "================================================"
echo "⚠️  确保已经用 cyh_rebuild_for_uftrace.sh 重新编译！"
echo ""
echo "📊 追踪策略："
echo "   - 使用 Debug 模式编译的二进制"
echo "   - 自动检测所有函数调用"
echo "   - 过滤系统库和短时间函数"
echo ""
echo "💡 提示："
echo "   - 等 UE 完成注册和 PDU Session 建立后，按 Ctrl+C 停止"
echo "   - 或者运行 30 秒后自动停止"
echo ""
read -p "按 Enter 继续..."

cd ~/openairinterface5g/cmake_targets/ran_build/build

# 创建输出目录
TRACE_DIR="/tmp/oai_gnb_trace_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$TRACE_DIR"

# 设置 uftrace 库路径
export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/uftrace:$LD_LIBRARY_PATH

echo ""
echo "================================================"
echo "开始追踪 gNB（30秒后自动停止）..."
echo "================================================"

# 使用 timeout 限制运行时间，避免 trace 太大
sudo -E timeout 30s uftrace record \
  -d "$TRACE_DIR" \
  --no-libcall \
  -D 15 \
  -t 10us \
  -N 'std::*' \
  -N '__*' \
  -N '_*' \
  ./nr-softmodem \
    -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf \
    --gNBs.[0].min_rxtxtime 6 \
    --rfsim || true

echo ""
echo "================================================"
echo "✅ Trace 数据已保存到: $TRACE_DIR"
echo "================================================"
echo ""
echo "📊 快速查看调用树："
echo "   uftrace replay -d $TRACE_DIR --no-libcall -D 15 -t 10us | head -1000"
echo ""
echo "🔍 查看 RRC 相关函数："
echo "   uftrace replay -d $TRACE_DIR --no-libcall -D 20 -t 1us | grep -i rrc | head -500"
echo ""
echo "🔍 查看 NGAP 相关函数："
echo "   uftrace replay -d $TRACE_DIR --no-libcall -D 20 -t 1us | grep -i ngap | head -500"
echo ""
echo "🎨 生成 Chrome Trace (时间线)："
echo "   uftrace dump -d $TRACE_DIR --chrome > $TRACE_DIR/gnb_trace.json"
echo ""
echo "或使用 ./cyh_view_trace.sh 交互式查看"
echo ""
