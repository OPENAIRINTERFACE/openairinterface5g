#!/usr/bin/env bash
# 使用 uftrace 动态追踪 gNB（不需要重新编译）
# 使用 -P 选项进行动态插桩

set -e

echo "================================================"
echo "使用 uftrace 动态追踪 gNB"
echo "================================================"
echo "✅ 使用动态追踪模式（-P），无需重新编译"
echo ""
echo "📊 追踪策略："
echo "   - 动态插桩（运行时注入）"
echo "   - 只追踪特定函数模式"
echo "   - 过滤系统库和短时间函数"
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

# 设置库路径（包括 OAI 的库）
export LD_LIBRARY_PATH=$(pwd):$LD_LIBRARY_PATH

echo ""
echo "================================================"
echo "开始动态追踪 gNB（30秒后自动停止）..."
echo "================================================"

# 使用 bash -c 包装，避免 sudo 丢失环境变量
sudo bash -c "
export LD_LIBRARY_PATH=$(pwd):\$LD_LIBRARY_PATH
timeout 30s uftrace record \
  -d '$TRACE_DIR' \
  -P 'nr_rrc*' \
  -P 'ngap*' \
  -P 'itti*' \
  -P 'nas*' \
  -P 'rrc*' \
  --no-libcall \
  -D 15 \
  -t 10us \
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
echo "📊 快速查看调用树："
echo "   uftrace replay -d $TRACE_DIR --no-libcall -D 15 -t 5us"
echo ""
echo "🔍 查看 RRC 相关函数："
echo "   uftrace replay -d $TRACE_DIR --no-libcall -D 20 -t 1us | grep -i rrc"
echo ""
echo "🔍 查看 NGAP 相关函数："
echo "   uftrace replay -d $TRACE_DIR --no-libcall -D 20 -t 1us | grep -i ngap"
echo ""
echo "💾 Trace 目录: $TRACE_DIR"
echo ""
