#!/usr/bin/env bash
# 分析 Registration Reject 场景的 trace

set -e

if [ -z "$1" ]; then
  TRACE_DIR=$(ls -dt /tmp/oai_gnb_trace_* 2>/dev/null | head -1)
else
  TRACE_DIR=$1
fi

if [ ! -d "$TRACE_DIR" ]; then
  echo "❌ 目录不存在: $TRACE_DIR"
  exit 1
fi

echo "================================================"
echo "分析 Registration Reject 场景"
echo "================================================"
echo "📍 Trace 目录: $TRACE_DIR"
echo ""

OUTPUT="$TRACE_DIR/registration_reject_analysis.txt"

echo "🔍 查找 Registration Reject 相关函数..."
echo ""

# 提取完整的 NGAP Downlink NAS 流程（包含 Reject）
uftrace replay -d "$TRACE_DIR" --no-libcall -D 30 -t 0us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | \
  grep -B 5 -A 50 'rrc_gNB_process_NGAP_DOWNLINK_NAS' | \
  head -300 > "$OUTPUT"

echo "✅ 已保存到: $OUTPUT"
echo ""
echo "================================================"
echo "📊 分析结果："
echo "================================================"
echo ""

# 统计 Downlink NAS 的次数
DOWNLINK_COUNT=$(grep -c 'rrc_gNB_process_NGAP_DOWNLINK_NAS' "$OUTPUT" || echo "0")

echo "找到 ${DOWNLINK_COUNT} 次 NGAP Downlink NAS 处理"
echo ""

if [ "$DOWNLINK_COUNT" -eq 0 ]; then
  echo "❌ 没有找到 Downlink NAS 处理记录"
  echo ""
  echo "可能原因："
  echo "  1. UE 在 RRC Setup 阶段就失败了（检查 step1）"
  echo "  2. gNB 没有收到 AMF 的响应（网络问题）"
  echo "  3. Trace 时间太短，没有捕获到"
elif [ "$DOWNLINK_COUNT" -eq 1 ]; then
  echo "⚠️  只有 1 次 Downlink NAS → 可能是 Registration Reject"
  echo ""
  echo "正常流程应该有 3-4 次 Downlink NAS："
  echo "  1. Authentication Request"
  echo "  2. Security Mode Command"
  echo "  3. Registration Accept"
  echo "  4. (可选) Configuration Update"
  echo ""
  echo "如果只有 1 次，很可能是直接 Reject"
else
  echo "✅ 有 ${DOWNLINK_COUNT} 次 Downlink NAS → 可能是正常流程"
  echo ""
  echo "第 1 次: Authentication Request"
  echo "第 2 次: Security Mode Command"
  echo "第 3 次: Registration Accept (或 Reject)"
fi

echo ""
echo "================================================"
echo "🔍 详细分析："
echo "================================================"
echo ""
echo "查看前 100 行 Downlink NAS 处理："
head -100 "$OUTPUT"

echo ""
echo "================================================"
echo "💡 判断是否是 Registration Reject："
echo "================================================"
echo ""
echo "1️⃣  查看 Downlink NAS 次数："
echo "   正常: 3-4 次（认证 → 安全 → 接受）"
echo "   Reject: 1-2 次（可能直接拒绝，或认证后拒绝）"
echo ""
echo "2️⃣  查看是否有后续的 PDU Session Setup："
echo "   grep 'PDUSESSION_SETUP' $OUTPUT"
echo "   - 如果没有，说明注册失败"
echo ""
echo "3️⃣  查看 Core 日志："
echo "   在 AMF 日志中搜索 'Registration Reject' 或 'Tracking area not allowed'"
echo ""
echo "4️⃣  查看 UE 日志："
echo "   搜索 '[NAS] Received Registration reject'"
echo ""
