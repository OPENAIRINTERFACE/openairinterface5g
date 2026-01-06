#!/usr/bin/env bash
# 生成 UE 注册流程的详细调用树（带上下文）

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
echo "生成 UE 注册流程详细调用树"
echo "================================================"
echo "📍 Trace 目录: $TRACE_DIR"
echo ""

# 1. RRC Setup Request 处理
echo "🔍 1. 提取【UE → gNB】RRC Setup Request 处理流程..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 30 -t 0us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | \
  grep -A 50 'rrc_gNB_process_initial_ul_rrc_message' | head -60 \
  > "$TRACE_DIR/step1_rrc_setup_request.txt"
echo "   ✅ 已保存到: $TRACE_DIR/step1_rrc_setup_request.txt"

# 2. RRC Setup Complete 处理
echo "🔍 2. 提取【UE → gNB】RRC Setup Complete 处理流程..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 30 -t 0us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | \
  grep -A 50 'rrc_gNB_process_RRCSetupComplete' | head -60 \
  > "$TRACE_DIR/step2_rrc_setup_complete.txt"
echo "   ✅ 已保存到: $TRACE_DIR/step2_rrc_setup_complete.txt"

# 3. NGAP Downlink NAS 处理
echo "🔍 3. 提取【Core → gNB】NGAP Downlink NAS 处理流程..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 30 -t 0us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | \
  grep -A 40 'rrc_gNB_process_NGAP_DOWNLINK_NAS' | head -100 \
  > "$TRACE_DIR/step3_ngap_downlink_nas.txt"
echo "   ✅ 已保存到: $TRACE_DIR/step3_ngap_downlink_nas.txt"

# 4. PDU Session Setup 处理
echo "🔍 4. 提取【Core → gNB】PDU Session Setup 处理流程..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 30 -t 0us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | \
  grep -A 80 'rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ' | head -100 \
  > "$TRACE_DIR/step4_pdu_session_setup.txt"
echo "   ✅ 已保存到: $TRACE_DIR/step4_pdu_session_setup.txt"

# 5. RRC Reconfiguration Complete 处理
echo "🔍 5. 提取【UE → gNB】RRC Reconfiguration Complete 处理流程..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 30 -t 0us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | \
  grep -A 60 'rrc_CU_process_ue_context_setup_response' | head -80 \
  > "$TRACE_DIR/step5_rrc_reconfig_complete.txt"
echo "   ✅ 已保存到: $TRACE_DIR/step5_rrc_reconfig_complete.txt"

echo ""
echo "================================================"
echo "✅ 完成！生成了 5 个关键流程文件"
echo "================================================"
echo ""
echo "📁 按顺序查看 UE 注册流程："
echo ""
echo "1️⃣  【UE → gNB】RRC Setup Request"
echo "   less $TRACE_DIR/step1_rrc_setup_request.txt"
echo ""
echo "2️⃣  【UE → gNB】RRC Setup Complete + NAS Registration"
echo "   less $TRACE_DIR/step2_rrc_setup_complete.txt"
echo ""
echo "3️⃣  【Core → gNB】Downlink NAS（认证/安全）"
echo "   less $TRACE_DIR/step3_ngap_downlink_nas.txt"
echo ""
echo "4️⃣  【Core → gNB】PDU Session Setup"
echo "   less $TRACE_DIR/step4_pdu_session_setup.txt"
echo ""
echo "5️⃣  【UE → gNB】RRC Reconfiguration Complete"
echo "   less $TRACE_DIR/step5_rrc_reconfig_complete.txt"
echo ""
echo "================================================"
echo "💡 提示："
echo "================================================"
echo ""
echo "每个文件显示的是 gNB 接收到消息后的完整处理流程，包括："
echo "  - 消息解码（uper_decode）"
echo "  - 业务逻辑处理（rrc_xxx, ngap_xxx）"
echo "  - 响应消息生成（generate_xxx）"
echo "  - 消息发送（dl_rrc_message_transfer）"
echo ""
echo "通过这些文件，您可以清楚看到 gNB 在每个阶段的处理逻辑！"
echo ""
