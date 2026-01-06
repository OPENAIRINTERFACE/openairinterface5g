# UE注册流程分析指南

## 完整的UE注册流程（从SIB1到PDU Session建立）

### 阶段1: UE接收SIB1并选择PLMN

**UE侧函数**：
```
nr_rrc_process_sib1()  // openair2/RRC/NR_UE/rrc_UE.c
  ├─ 解析SIB1中的plmn_IdentityInfoList
  ├─ 遍历PLMN列表，与UE IMSI匹配
  ├─ 匹配成功：设置 rrc->selected_plmn_identity = j + 1 (1-based索引)
  └─ 匹配失败：fallback到 selected_plmn_identity = 1 (❌ Bug所在)
```

**关键变量**：
- `rrc->selected_plmn_identity` - UE选择的PLMN索引（1-based）

---

### 阶段2: UE发送RRC Setup Complete

**UE侧函数**：
```
do_NR_RRCSetupComplete()  // openair2/RRC/NR_UE/rrc_UE.c
  └─ nr_rrc_ue_generate_rrcSetupComplete()  // openair2/RRC/NR/MESSAGES/asn1_msg.c
       ├─ 填充 ies->selectedPLMN_Identity = rrc->selected_plmn_identity
       ├─ 填充 dedicatedNAS_Message (NAS Registration Request)
       └─ 发送给gNB
```

**消息内容**：
- `selectedPLMN_Identity`: UE选择的PLMN索引
- `dedicatedNAS_Message`: NAS Registration Request（包含UE的PLMN、IMSI等）

---

### 阶段3: gNB接收并处理RRC Setup Complete

**gNB侧函数**：
```
rrc_gNB_process_RRCSetupComplete()  // openair2/RRC/NR/rrc_gNB.c:604
  └─ rrc_gNB_send_NGAP_NAS_FIRST_REQ()  // openair2/RRC/NR/rrc_gNB_NGAP.c:226
       ├─ 提取索引: idx = rrcSetupComplete->selectedPLMN_Identity - 1
       ├─ 验证边界: if (idx < 0 || idx >= num_plmn) return;
       ├─ 从配置提取PLMN: req->plmn = rrc->configuration.plmn[idx]
       ├─ 填充NAS PDU: req->nas_pdu = rrcSetupComplete->dedicatedNAS_Message
       ├─ 填充Cell ID、TAI等信息
       └─ 发送ITTI消息到NGAP任务
```

**关键逻辑**（Bug修复前后对比）：
```c
// ✅ 正确：使用UE选择的索引
int idx = rrcSetupComplete->selectedPLMN_Identity - 1;
req->plmn = rrc->configuration.plmn[idx];

// ❌ 错误（如果UE侧用了错误的索引i而不是j）：
// 可能选择了plmn[0]而不是UE实际想要的PLMN
```

---

### 阶段4: gNB构造NGAP Initial UE Message发给AMF

**gNB侧函数**：
```
ngap_gNB_handle_nas_first_req()  // openair3/NGAP/ngap_gNB_nas_procedures.c
  └─ ngap_generate_initial_ue_message()  // openair3/NGAP/ngap_gNB_nas_procedures.c
       ├─ 填充 InitialUEMessage
       │    ├─ NAS-PDU (UE的Registration Request)
       │    ├─ RAN-UE-NGAP-ID
       │    ├─ User Location Information (NR CGI + TAI)
       │    │    ├─ PLMN Identity (从gNB配置提取)
       │    │    └─ TAC (Tracking Area Code)
       │    └─ RRC Establishment Cause
       └─ 通过SCTP发送给AMF
```

**重要字段**：
- `Selected PLMN`: gNB从`plmn[idx]`提取的PLMN
- `TAI`: Tracking Area Identity = PLMN + TAC
- 如果PLMN选择错误（Bug），AMF会检查TAI并可能返回"Tracking area not allowed"

---

### 阶段5: AMF处理并响应

**AMF侧逻辑**（不在OAI代码中，通常在Core代码）：
```
收到 Initial UE Message
  ├─ 验证 Selected PLMN 是否在AMF支持的列表中
  ├─ 验证 TAI (PLMN + TAC) 是否允许
  ├─ 解析 NAS Registration Request
  │    ├─ 提取UE的PLMN、IMSI
  │    └─ 验证UE请求的PLMN是否与Selected PLMN一致
  │
  ├─ 成功：
  │    └─ 发送 Downlink NAS Transport (Registration Accept)
  │
  └─ 失败：
       └─ 发送 Downlink NAS Transport (Registration Reject)
            └─ Cause: "Tracking area not allowed" (如果PLMN/TAC不匹配)
```

---

### 阶段6: gNB接收AMF响应

**gNB侧函数**：
```
rrc_gNB_process_NGAP_DOWNLINK_NAS()  // openair2/RRC/NR/rrc_gNB_NGAP.c
  ├─ 接收 NGAP Downlink NAS Transport
  ├─ 提取 NAS PDU (Registration Accept 或 Reject)
  └─ 构造 RRC DL Information Transfer 转发给UE
       └─ 包含 NAS PDU
```

**成功流程**（Registration Accept）：
```
rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ()
  ├─ 设置安全上下文
  ├─ 发送 RRC Security Mode Command
  └─ 等待 RRC Security Mode Complete

rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ()
  ├─ 配置DRB (Data Radio Bearer)
  ├─ 发送 RRC Reconfiguration
  └─ 等待 RRC Reconfiguration Complete
```

---

## 如何使用uftrace分析这个流程

### 方法1: 使用快速追踪脚本

```bash
cd ~/openairinterface5g

# 1. 启动追踪脚本（会自动等待UE连接）
./cyh_quick_registration_trace.sh

# 2. 在另一个终端启动UE
./cyh_start_ue.sh

# 3. 等待30秒后自动生成分析报告
```

### 方法2: 手动分析现有trace

如果你已经有trace目录：

```bash
# 分析指定的trace目录
./cyh_analyze_ue_registration_flow.sh /tmp/oai_gnb_trace_XXXXXX

# 查看生成的分析文件
ls /tmp/oai_gnb_trace_XXXXXX/registration_analysis/
  01_ue_sib1_and_selection.txt       - UE接收SIB1并选择PLMN
  02_ue_rrc_setup_complete.txt       - UE发送RRC Setup Complete
  03_gnb_process_setup_complete.txt  - gNB处理Setup Complete
  04_gnb_to_core_initial_ue_msg.txt  - gNB发送Initial UE Message
  05_gnb_receive_core_response.txt   - gNB接收Core响应
  06_complete_timeline.txt           - 完整时间线
  07_call_tree.txt                   - 函数调用树
```

### 方法3: 手动uftrace命令

```bash
TRACE_DIR="/tmp/oai_gnb_trace_XXXXXX"

# 1. 查看UE PLMN选择
uftrace replay -d $TRACE_DIR -F 'nr_rrc_process_sib1'

# 2. 查看gNB处理RRC Setup Complete
uftrace replay -d $TRACE_DIR -F 'rrc_gNB_process_RRCSetupComplete'

# 3. 查看gNB发送到Core的消息
uftrace replay -d $TRACE_DIR -F 'rrc_gNB_send_NGAP_NAS_FIRST_REQ'

# 4. 查看gNB接收Core响应
uftrace replay -d $TRACE_DIR -F 'rrc_gNB_process_NGAP_DOWNLINK_NAS'

# 5. 查看完整调用树（以gNB处理Setup Complete为起点）
uftrace replay -d $TRACE_DIR -F 'rrc_gNB_process_RRCSetupComplete' --column-view

# 6. 查看函数调用时间统计
uftrace report -d $TRACE_DIR -F 'rrc_gNB_*|ngap_*'
```

---

## 关键观察点

### 验证PLMN选择是否正确

**查看UE日志**：
```bash
grep -E "\[cyhtest\].*PLMN matched at index" <ue_log_file>
```

期望输出（PLMN列表为`{460-11, 208-93}`，UE IMSI为208-93）：
```
[cyhtest] PLMN matched at index 1, set selected_plmn_identity=2
```

**查看gNB日志**：
```bash
grep -E "Selected PLMN in the NG Initial UE Message" <gnb_log_file>
```

期望输出：
```
Selected PLMN in the NG Initial UE Message: MCC=208 MNC=93
```

### 验证注册成功/失败

**成功标志**：
```bash
grep -E "NGAP_INITIAL_CONTEXT_SETUP|NGAP_PDUSESSION_SETUP" <gnb_log_file>
```

**失败标志**：
```bash
grep -E "Registration Reject|Tracking area not allowed" <gnb_log_file>
```

---

## Bug修复总结

**修复前**（错误）：
```c
// openair2/RRC/NR_UE/rrc_UE.c:569
rrc->selected_plmn_identity = i + 1;  // ❌ 使用外层索引
```

**修复后**（正确）：
```c
// openair2/RRC/NR_UE/rrc_UE.c:569
rrc->selected_plmn_identity = j + 1;  // ✅ 使用内层索引
```

**影响**：
- 修复前：UE总是选择`plmn_IdentityInfoList[i]`的索引，可能不正确
- 修复后：UE正确选择`plmn_IdentityList[j]`的索引，对应实际匹配的PLMN

