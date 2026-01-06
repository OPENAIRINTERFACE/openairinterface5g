#!/usr/bin/env bash
# 使用 uftrace 追踪插桩编译的 gNB

set -e

echo "================================================"
echo "使用 uftrace 追踪 gNB（插桩版本）"
echo "================================================"
echo "⚠️  确保已经用 cyh_rebuild_with_instrument.sh 重新编译！"
echo ""
echo "💡 提示："
echo "   - 等 UE 完成注册后，按 Ctrl+C 停止"
echo "   - 或者运行 30 秒后自动停止"
echo ""
read -p "按 Enter 继续..."

cd ~/openairinterface5g/cmake_targets/ran_build/build

# 创建输出目录
TRACE_DIR="/tmp/oai_gnb_trace_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$TRACE_DIR"

# 设置库路径（包括 uftrace 和 OAI 的库）
export LD_LIBRARY_PATH=$(pwd):/usr/lib/x86_64-linux-gnu/uftrace:$LD_LIBRARY_PATH

echo ""
echo "================================================"
echo "开始追踪 gNB（30秒后自动停止）..."
echo "================================================"

# 使用 bash -c 包装，避免 sudo 丢失环境变量
sudo bash -c "
export LD_LIBRARY_PATH=$(pwd):/usr/lib/x86_64-linux-gnu/uftrace:\$LD_LIBRARY_PATH
timeout 30s uftrace record \
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
echo "✅ Trace 数据已保存到: $TRACE_DIR"
echo "================================================"
echo ""
echo "📊 快速查看调用树（前 500 行）："
echo "   uftrace replay -d $TRACE_DIR --no-libcall -D 12 -t 10us | head -500"
echo ""
echo "🔍 查看 RRC 相关："
echo "   uftrace replay -d $TRACE_DIR --no-libcall -D 15 | grep -E 'rrc|RRC' | head -300"
echo ""
echo "🔍 查看 NGAP 相关："
echo "   uftrace replay -d $TRACE_DIR --no-libcall -D 15 | grep -E 'ngap|NGAP' | head -300"
echo ""
echo "💾 Trace 目录: $TRACE_DIR"
echo ""
