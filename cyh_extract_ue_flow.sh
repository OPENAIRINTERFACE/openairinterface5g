#!/usr/bin/env bash
# 提取 UE 注册流程的关键函数调用（按时间顺序）

set -e

if [ -z "$1" ]; then
  TRACE_DIR=$(ls -dt /tmp/oai_gnb_trace_* 2>/dev/null | head -1)
  if [ -z "$TRACE_DIR" ]; then
    echo "❌ 没有找到 trace 数据"
    exit 1
  fi
else
  TRACE_DIR=$1
fi

if [ ! -d "$TRACE_DIR" ]; then
  echo "❌ 目录不存在: $TRACE_DIR"
  exit 1
fi

echo "================================================"
echo "提取 UE 注册流程关键函数"
echo "================================================"
echo "📍 Trace 目录: $TRACE_DIR"
echo ""

OUTPUT="$TRACE_DIR/ue_registration_flow.txt"

echo "🔍 正在分析 UE 注册流程..."
echo ""

# 提取关键函数的时间线
uftrace replay -d "$TRACE_DIR" --no-libcall -D 30 -t 0us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | \
  grep -E 'rrc_gNB_process_initial_ul_rrc_message|rrc_handle_RRCSetupRequest|rrc_gNB_generate_RRCSetup|rrc_gNB_process_RRCSetupComplete|rrc_gNB_process_NGAP_DOWNLINK_NAS|rrc_gNB_process_NGAP_PDUSESSION_SETUP|ngap_gNB_handle_initial_context_request|ngap_generate_initial_ue_message|nr_rrc_reconfiguration_req|rrc_gNB_generate_UeContextSetupRequest|rrc_CU_process_ue_context_setup_response' \
  > "$OUTPUT"

echo "✅ 已保存到: $OUTPUT"
echo ""
echo "================================================"
echo "📊 UE 注册流程关键节点："
echo "================================================"
echo ""

# 分析并显示关键步骤
cat "$OUTPUT" | head -50

echo ""
echo "================================================"
echo "🔍 详细说明："
echo "================================================"
echo ""
echo "1️⃣  【UE → gNB】RRC Setup Request"
echo "   函数: rrc_gNB_process_initial_ul_rrc_message()"
echo "   说明: gNB 接收到 UE 的 RRC Setup Request"
echo ""
echo "2️⃣  【gNB 处理】处理 RRC Setup Request"
echo "   函数: rrc_handle_RRCSetupRequest()"
echo "   说明: gNB 解析 UE 的请求，分配 RNTI"
echo ""
echo "3️⃣  【gNB → UE】发送 RRC Setup"
echo "   函数: rrc_gNB_generate_RRCSetup()"
echo "   说明: gNB 生成并发送 RRC Setup 消息"
echo ""
echo "4️⃣  【UE → gNB】RRC Setup Complete（含 NAS Registration Request）"
echo "   函数: rrc_gNB_process_RRCSetupComplete()"
echo "   说明: UE 完成 RRC 连接，发送 NAS 注册请求"
echo ""
echo "5️⃣  【gNB → Core】Initial UE Message"
echo "   函数: ngap_generate_initial_ue_message()"
echo "   说明: gNB 转发 NAS 消息到 AMF"
echo ""
echo "6️⃣  【Core → gNB】Downlink NAS（认证、安全等）"
echo "   函数: rrc_gNB_process_NGAP_DOWNLINK_NAS()"
echo "   说明: AMF 下发认证/安全消息（可能多次）"
echo ""
echo "7️⃣  【Core → gNB】PDU Session Setup Request"
echo "   函数: rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ()"
echo "   说明: SMF 请求建立 PDU Session"
echo ""
echo "8️⃣  【gNB → UE】RRC Reconfiguration"
echo "   函数: rrc_gNB_generate_UeContextSetupRequest()"
echo "   说明: gNB 发送 RRC Reconfiguration，建立 DRB"
echo ""
echo "9️⃣  【UE → gNB】RRC Reconfiguration Complete"
echo "   函数: rrc_CU_process_ue_context_setup_response()"
echo "   说明: UE 确认 RRC Reconfiguration，PDU Session 建立完成"
echo ""
echo "================================================"
echo "💡 查看完整流程："
echo "   cat $OUTPUT | less"
echo ""
echo "💡 查看特定函数的上下文（例如 RRCSetupRequest）："
echo "   uftrace replay -d $TRACE_DIR --no-libcall -D 30 -t 0us \\"
echo "     -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | \\"
echo "     grep -A 20 -B 10 'rrc_handle_RRCSetupRequest' | less"
echo ""
