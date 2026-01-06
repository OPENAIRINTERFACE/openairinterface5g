#!/usr/bin/env bash
# 分析UE注册完整流程：从SIB1接收到Core响应
# 用法: ./cyh_analyze_ue_registration_flow.sh <trace_dir>

set -e

if [ $# -lt 1 ]; then
    echo "用法: $0 <trace_directory>"
    echo ""
    echo "示例: $0 /tmp/oai_gnb_trace_20251222_135612"
    echo ""
    echo "如果没有现成的trace，请先运行:"
    echo "  ./cyh_trace_full_workflow.sh"
    exit 1
fi

TRACE_DIR="$1"

if [ ! -d "$TRACE_DIR" ]; then
    echo "❌ 错误：trace目录不存在: $TRACE_DIR"
    exit 1
fi

echo "================================================"
echo "分析UE注册完整流程"
echo "================================================"
echo "Trace目录: $TRACE_DIR"
echo ""

# 输出目录
ANALYSIS_DIR="${TRACE_DIR}/registration_analysis"
mkdir -p "$ANALYSIS_DIR"

echo "步骤1: 提取UE侧关键函数"
echo "----------------------------------------"
cat > "${ANALYSIS_DIR}/01_ue_sib1_and_selection.txt" <<'HEADER'
=== UE侧：接收SIB1并选择PLMN ===

关键流程：
1. UE接收SIB1广播（包含PLMN列表）
2. UE解析SIB1中的PLMN信息
3. UE根据IMSI匹配PLMN
4. UE设置selectedPLMN_Identity

HEADER

uftrace replay -d "$TRACE_DIR" -F 'nr_rrc_process_sib1' --column-view 2>/dev/null | head -100 >> "${ANALYSIS_DIR}/01_ue_sib1_and_selection.txt" || echo "未找到UE SIB1处理函数"

echo "✅ 已生成: 01_ue_sib1_and_selection.txt"

echo ""
echo "步骤2: 提取UE发送RRC Setup Complete"
echo "----------------------------------------"
cat > "${ANALYSIS_DIR}/02_ue_rrc_setup_complete.txt" <<'HEADER'
=== UE侧：发送RRC Setup Complete（包含selectedPLMN_Identity）===

关键流程：
1. UE构造RRCSetupComplete消息
2. 消息中包含：
   - selectedPLMN_Identity (UE选择的PLMN索引)
   - NAS Registration Request
3. UE通过RRC层发送给gNB

HEADER

uftrace replay -d "$TRACE_DIR" -F 'do_NR_RRCSetupComplete|nr_rrc_ue_generate_rrcSetupComplete' --column-view 2>/dev/null | head -100 >> "${ANALYSIS_DIR}/02_ue_rrc_setup_complete.txt" || echo "未找到UE RRCSetupComplete函数"

echo "✅ 已生成: 02_ue_rrc_setup_complete.txt"

echo ""
echo "步骤3: 提取gNB接收并处理RRC Setup Complete"
echo "----------------------------------------"
cat > "${ANALYSIS_DIR}/03_gnb_process_setup_complete.txt" <<'HEADER'
=== gNB侧：接收并处理RRC Setup Complete ===

关键流程：
1. gNB接收UE的RRCSetupComplete
2. 解析selectedPLMN_Identity字段
3. 根据索引从plmn_list中提取对应PLMN
4. 调用rrc_gNB_send_NGAP_NAS_FIRST_REQ()

重要代码位置：
- openair2/RRC/NR/rrc_gNB.c: rrc_gNB_process_RRCSetupComplete()
- openair2/RRC/NR/rrc_gNB_NGAP.c: rrc_gNB_send_NGAP_NAS_FIRST_REQ()

HEADER

uftrace replay -d "$TRACE_DIR" -F 'rrc_gNB_process_RRCSetupComplete|rrc_gNB_send_NGAP_NAS_FIRST_REQ' --column-view 2>/dev/null >> "${ANALYSIS_DIR}/03_gnb_process_setup_complete.txt" || echo "未找到gNB处理函数"

echo "✅ 已生成: 03_gnb_process_setup_complete.txt"

echo ""
echo "步骤4: 提取gNB构造Initial UE Message发给Core"
echo "----------------------------------------"
cat > "${ANALYSIS_DIR}/04_gnb_to_core_initial_ue_msg.txt" <<'HEADER'
=== gNB侧：构造NGAP Initial UE Message发给AMF ===

关键流程：
1. gNB从UE选择的PLMN索引提取PLMN信息
2. 构造NGAP_NAS_FIRST_REQ消息
3. 填充：
   - Selected PLMN (从plmn[idx]提取)
   - NAS PDU (UE的Registration Request)
   - NR Cell ID
   - TAI (Tracking Area Identity)
4. 通过NGAP发送给AMF

重要字段：
- req->plmn = rrc->configuration.plmn[idx]
- idx = rrcSetupComplete->selectedPLMN_Identity - 1

HEADER

uftrace replay -d "$TRACE_DIR" -F 'ngap_generate_initial_ue_message|ngap_gNB_handle_nas_first_req' --column-view 2>/dev/null >> "${ANALYSIS_DIR}/04_gnb_to_core_initial_ue_msg.txt" || echo "未找到NGAP Initial UE Message函数"

echo "✅ 已生成: 04_gnb_to_core_initial_ue_msg.txt"

echo ""
echo "步骤5: 提取gNB接收Core的响应"
echo "----------------------------------------"
cat > "${ANALYSIS_DIR}/05_gnb_receive_core_response.txt" <<'HEADER'
=== gNB侧：接收AMF的响应 ===

可能的响应类型：
1. NGAP_DOWNLINK_NAS (包含NAS消息，如Registration Accept/Reject)
2. NGAP_INITIAL_CONTEXT_SETUP_REQ (如果注册成功)
3. NGAP_PDUSESSION_SETUP_REQ (PDU Session建立)

Registration Accept流程：
- AMF -> gNB: Downlink NAS Transport (Registration Accept)
- gNB -> UE: RRC DL Information Transfer (转发NAS消息)

Registration Reject流程：
- AMF -> gNB: Downlink NAS Transport (Registration Reject)
- gNB -> UE: RRC DL Information Transfer (转发NAS消息)

HEADER

uftrace replay -d "$TRACE_DIR" -F 'rrc_gNB_process_NGAP_DOWNLINK_NAS|rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ|rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ' --column-view 2>/dev/null >> "${ANALYSIS_DIR}/05_gnb_receive_core_response.txt" || echo "未找到gNB处理Core响应的函数"

echo "✅ 已生成: 05_gnb_receive_core_response.txt"

echo ""
echo "步骤6: 提取完整时间线"
echo "----------------------------------------"
cat > "${ANALYSIS_DIR}/06_complete_timeline.txt" <<'HEADER'
=== 完整UE注册时间线 ===

从UE接收SIB1到PDU Session建立的完整流程

HEADER

# 提取所有关键函数的调用时间线
uftrace replay -d "$TRACE_DIR" -F 'nr_rrc_process_sib1|rrc_gNB_process_RRCSetupComplete|rrc_gNB_send_NGAP_NAS_FIRST_REQ|ngap_generate_initial_ue_message|rrc_gNB_process_NGAP_DOWNLINK_NAS|rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ|rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ' 2>/dev/null >> "${ANALYSIS_DIR}/06_complete_timeline.txt" || echo "提取时间线时出现问题"

echo "✅ 已生成: 06_complete_timeline.txt"

echo ""
echo "步骤7: 生成函数调用树"
echo "----------------------------------------"
cat > "${ANALYSIS_DIR}/07_call_tree.txt" <<'HEADER'
=== 关键函数调用树 ===

显示嵌套调用关系

HEADER

uftrace replay -d "$TRACE_DIR" -F 'rrc_gNB_process_RRCSetupComplete' -t 100us --column-view 2>/dev/null >> "${ANALYSIS_DIR}/07_call_tree.txt" || echo "未找到调用树"

echo "✅ 已生成: 07_call_tree.txt"

echo ""
echo "================================================"
echo "✅ 分析完成！"
echo "================================================"
echo ""
echo "生成的文件："
ls -lh "${ANALYSIS_DIR}"/*.txt | awk '{print "  " $9 " (" $5 ")"}'
echo ""
echo "查看方式："
echo "  cat ${ANALYSIS_DIR}/01_ue_sib1_and_selection.txt"
echo "  cat ${ANALYSIS_DIR}/02_ue_rrc_setup_complete.txt"
echo "  cat ${ANALYSIS_DIR}/03_gnb_process_setup_complete.txt"
echo "  cat ${ANALYSIS_DIR}/04_gnb_to_core_initial_ue_msg.txt"
echo "  cat ${ANALYSIS_DIR}/05_gnb_receive_core_response.txt"
echo "  cat ${ANALYSIS_DIR}/06_complete_timeline.txt"
echo "  cat ${ANALYSIS_DIR}/07_call_tree.txt"
echo ""
echo "或者一次性查看所有："
echo "  less ${ANALYSIS_DIR}/*.txt"
echo ""
