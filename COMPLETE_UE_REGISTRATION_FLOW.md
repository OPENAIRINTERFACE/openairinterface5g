# UE注册完整流程详解
## 从SIB1广播到PDU Session建立

基于3GPP TS 38.331 (RRC), TS 38.413 (NGAP), TS 24.501 (5GC NAS)  
以及OpenAirInterface5G实现（Rel-15，向Rel-17迈进）

---

## 📋 前置条件

**gNB启动**:
```bash
cd ~/openairinterface5g/cmake_targets/ran_build/build
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf \
  --gNBs.[0].min_rxtxtime 6 --rfsim
```

**UE启动**:
```bash
cd ~/openairinterface5g/cmake_targets/ran_build/build
sudo ./nr-uesoftmodem --rfsim --rfsimulator.serveraddr 127.0.0.1 -r 106 \
  --numerology 1 --band 78 -C 3619200000 --ssb 516 \
  -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf
```

---

## 🔄 完整流程概览

```
时间线                gNB侧                                UE侧
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
T0: SIB1广播        [下行] SIB1 (PLMN列表)    ────────>    接收SIB1
                                                           └─ nr_rrc_process_sib1()
                                                              ├─ 解析PLMN列表
                                                              ├─ 与IMSI匹配
                                                              └─ selected_plmn_identity = j+1

T1: RRC连接请求      <────────  [上行] RRC Setup Request    发起连接
                    rrc_gNB_process_RRCSetupRequest()
                    └─ 分配C-RNTI
                    
T2: RRC连接建立      [下行] RRC Setup          ────────>    接收RRC Setup
                    rrc_gNB_generate_RRCSetup()             do_NR_RRCSetup()

T3: RRC完成+NAS      <────────  [上行] RRC Setup Complete   nr_rrc_ue_generate_
                    rrc_gNB_process_RRCSetupComplete()      rrcSetupComplete()
                    └─ rrc_gNB_send_NGAP_NAS_FIRST_REQ()    ├─ selectedPLMN_Identity
                       ├─ 提取UE选择的PLMN索引               └─ NAS Registration Req
                       └─ 发送ITTI消息到NGAP任务

T4: 初始UE消息       [N2] NGAP Initial UE Msg  ────────>    (到AMF)
                    ngap_gNB_handle_nas_first_req()
                    └─ ngap_generate_initial_ue_message()
                       ├─ Selected PLMN
                       ├─ NAS-PDU
                       ├─ RAN-UE-NGAP-ID
                       └─ User Location (TAI)

T5: 注册接受         <────────  [N2] Downlink NAS          (AMF处理)
                    rrc_gNB_process_NGAP_DOWNLINK_NAS()
                    └─ rrc_forward_ue_nas_message()
                       [下行] DL Information Transfer ──>   NAS Registration Accept

T6: 安全模式         <────────  [N2] Initial Context Setup
                    rrc_gNB_process_NGAP_INITIAL_CONTEXT_
                    SETUP_REQ()
                    ├─ set_UE_security_algos()
                    ├─ nr_rrc_pdcp_config_security()
                    │  └─ nr_derive_key() (2次)
                    └─ rrc_gNB_generate_SecurityModeCommand()
                       [下行] Security Mode Command ────>   
                       <────────  [上行] Security Mode Complete

T7: PDU会话建立      <────────  [N2] PDU Session Setup Req
                    rrc_gNB_process_NGAP_PDUSESSION_
                    SETUP_REQ()
                    └─ trigger_bearer_setup()
                       ├─ add_pduSession()
                       ├─ nr_derive_key() (2次)
                       ├─ nr_rrc_add_drb()
                       └─ cucp_cuup_bearer_context_setup()
                       [下行] RRC Reconfiguration ──────>   
                       <────────  [上行] RRC Reconfiguration Complete

T8: 完成             UE注册成功，获得IP地址                ping成功 ✅
```

---

## 📖 详细步骤分析

### 阶段0: SIB1广播（在UE连接之前就在持续广播）

#### gNB侧关键函数

**1. 配置阶段（启动时）**
```c
// openair2/GNB_APP/gnb_config.c
RCconfig_NR_NG()
  └─ RCconfig_NRRRC()  // 读取配置文件
     └─ 读取 gNBs.[0].plmn_list
        配置示例：
        plmn_list = (
          { mcc = 208; mnc = 93; ... },
          { mcc = 460; mnc = 11; ... }
        );
```

**2. MAC层SIB1配置**
```c
// openair2/LAYER2/NR_MAC_gNB/config.c:1113
nr_mac_configure_sib1(gNB_MAC_INST *nrmac, int CC_id, int p, int f)
  └─ get_SIB1_NR(scc, plmn, cellID, tac, &nrmac->radio_config, 
                 num_plmn, plmn_list)  
     // openair2/LAYER2/NR_MAC_gNB/nr_radio_config.c:2710
     ├─ 创建NR_BCCH_DL_SCH_Message
     ├─ 填充cellSelectionInfo (q_RxLevMin)
     ├─ 填充cellAccessRelatedInfo
     │  └─ plmn_IdentityInfoList
     │     └─ plmn_IdentityList  // 存放多个PLMN
     │        ├─ plmn[0]: MCC=208, MNC=93
     │        └─ plmn[1]: MCC=460, MNC=11
     ├─ 填充servingCellConfigCommon
     └─ encode_SIB_NR()  // 编码成二进制
```

**3. RRC层SIB1处理**
```c
// openair2/RRC/NR/rrc_gNB_du.c
rrc_gNB_process_f1_setup_req()
  └─ extract_sys_info()  // 从F1AP提取SIB1
     └─ du->sib1 = sib1;  // 存储SIB1供后续使用
```

**4. 物理层调度SIB1广播**
```c
// openair2/LAYER2/NR_MAC_gNB/gNB_scheduler.c
nr_schedule_sib1()
  └─ 周期性在PDSCH上广播SIB1
     ├─ 频率：每160ms一次（标准定义）
     └─ 信道：BCCH-DL-SCH
```

#### UE侧关键函数

**1. UE接收SIB1并解析**
```c
// openair2/RRC/NR_UE/rrc_UE.c:520
nr_rrc_process_sib1(NR_UE_RRC_INST_t *rrc, NR_SIB1_t *sib1)
  ├─ 步骤1: 从NAS层获取UE的IMSI PLMN
  │   nas_proc_t *nas = get_ue_nas_info();
  │   uint8_t ue_mcc[3], ue_mnc[3];  
  │   // 从IMSI提取：例如IMSI=208930000000001 → MCC=208, MNC=93
  │
  ├─ 步骤2: 遍历SIB1中的PLMN列表
  │   for (int i = 0; i < plmn_IdentityInfoList.count; i++)  // 外层
  │     for (int j = 0; j < plmn_IdentityList.count; j++)   // 内层
  │       NR_PLMN_Identity_t *plmn_id = ...array[i]->...array[j]
  │       
  │       ├─ 比较MCC (3位数字)
  │       │   bool mcc_match = (plmn_id->mcc[0] == ue_mcc[0]) &&
  │       │                    (plmn_id->mcc[1] == ue_mcc[1]) &&
  │       │                    (plmn_id->mcc[2] == ue_mcc[2]);
  │       │
  │       └─ 比较MNC (2或3位数字)
  │           if (plmn_id->mnc.count == 2)
  │             mnc_match = (mnc_len == 2) &&
  │                         (plmn_id->mnc[0] == ue_mnc[1]) &&
  │                         (plmn_id->mnc[1] == ue_mnc[2]);
  │
  ├─ 步骤3: 匹配成功，设置selectedPLMN_Identity
  │   if (mcc_match && mnc_match)
  │     rrc->selected_plmn_identity = j + 1;  // ✅ 1-based索引
  │     // j是内层索引（plmn_IdentityList中的位置）
  │     // 例如：plmn_list={208-93, 460-11}, UE IMSI=208-93
  │     //      匹配到j=0，所以selected_plmn_identity=1
  │     plmn_matched = true;
  │     LOG_I(NR_RRC, "PLMN matched at index %d", j);
  │
  └─ 步骤4: 如果没有匹配（不应该发生）
      if (!plmn_matched)
        rrc->selected_plmn_identity = 1;  // fallback到第一个
```

**关键点**：
- SIB1的PLMN存储结构是**二维数组**
- OAI实现中通常只使用`plmn_IdentityInfoList[0]`，所有PLMN放在第一个IdentityInfo里
- 所以实际是`plmn_IdentityInfoList[0].plmn_IdentityList[j]`
- **BUG修复**：必须使用内层索引`j`，不是外层索引`i`

---

### 阶段1: RRC Setup Request (T1)

#### UE侧发起
```c
// openair2/RRC/NR_UE/rrc_UE.c
nr_rrc_ue_generate_rrcSetupRequest()
  └─ 触发条件：UE完成RACH过程后
     ├─ 填充establishment_cause (例如：mo-Data)
     ├─ 填充ue_Identity (S-TMSI或随机值)
     └─ 通过CCCH (SRB0)发送给gNB
```

#### gNB侧处理
```c
// openair2/RRC/NR/rrc_gNB.c:1900
rrc_gNB_decode_ccch()
  └─ case NR_UL_CCCH_MessageType__c1_PR_rrcSetupRequest:
     rrc_gNB_process_RRCSetupRequest()  // openair2/RRC/NR/rrc_gNB.c:380
       ├─ 分配UE上下文 (gNB_RRC_UE_t)
       ├─ 分配C-RNTI
       ├─ 记录establishment_cause
       └─ 调用 rrc_gNB_generate_RRCSetup()
```

---

### 阶段2: RRC Setup (T2)

#### gNB侧生成
```c
// openair2/RRC/NR/rrc_gNB.c:420
rrc_gNB_generate_RRCSetup(gNB_RRC_INST *rrc, gNB_RRC_UE_t *ue_p, ...)
  ├─ 创建RRCSetup消息
  ├─ 配置SRB1 (Signaling Radio Bearer 1)
  ├─ 配置默认的MAC/RLC/PDCP参数
  └─ 通过DCCH (SRB0)发送RRCSetup消息
     // openair2/RRC/NR/MESSAGES/asn1_msg.c
     do_RRCSetup()
       └─ 填充masterCellGroup配置
```

#### UE侧处理
```c
// openair2/RRC/NR_UE/rrc_UE.c
do_NR_RRCSetup(NR_UE_RRC_INST_t *rrc, NR_RRCSetup_t *rrcSetup)
  ├─ 解析masterCellGroup
  ├─ 配置SRB1
  ├─ 配置MAC/PHY参数
  └─ 准备发送RRCSetupComplete
```

---

### 阶段3: RRC Setup Complete + NAS Registration Request (T3) 🔑

**这是PLMN选择的关键阶段！**

#### UE侧生成
```c
// openair2/RRC/NR_UE/rrc_UE.c
do_NR_RRCSetupComplete()
  └─ nr_rrc_ue_generate_rrcSetupComplete()
     // openair2/RRC/NR/MESSAGES/asn1_msg.c:800
     ├─ 创建RRCSetupComplete消息
     ├─ ies->selectedPLMN_Identity = sel_plmn_id;
     │  // sel_plmn_id = rrc->selected_plmn_identity
     │  // 这是UE在nr_rrc_process_sib1()中设置的值！
     │  // 例如：如果UE选择了plmn_list[1] (208-93)
     │  //      则selectedPLMN_Identity = 2 (1-based)
     │
     ├─ ies->dedicatedNAS_Message = nas_pdu;
     │  // NAS Registration Request，包含：
     │  // - 5GS Registration Type
     │  // - SUCI/SUPI (包含IMSI的PLMN信息)
     │  // - UE Security Capability
     │  // - 5GMM Capability
     │
     └─ 通过SRB1发送给gNB
```

#### gNB侧处理（PLMN提取的关键步骤）
```c
// openair2/RRC/NR/rrc_gNB.c:604
rrc_gNB_process_RRCSetupComplete(gNB_RRC_INST *rrc, 
                                 gNB_RRC_UE_t *UE, 
                                 NR_RRCSetupComplete_IEs_t *rrcSetupComplete)
  ├─ UE->Srb[1].Active = 1;  // 激活SRB1
  └─ rrc_gNB_send_NGAP_NAS_FIRST_REQ(rrc, UE, rrcSetupComplete);

// openair2/RRC/NR/rrc_gNB_NGAP.c:226
rrc_gNB_send_NGAP_NAS_FIRST_REQ(...)
  ├─ 步骤1: 提取UE选择的PLMN索引
  │   int idx = rrcSetupComplete->selectedPLMN_Identity - 1;
  │   // 转换为0-based索引
  │   // 例如：UE发送selectedPLMN_Identity=2，idx=1
  │
  ├─ 步骤2: 验证索引范围
  │   if (idx < 0 || idx >= rrc->configuration.num_plmn)
  │     LOG_E(NGAP, "selected PLMN index out of bounds");
  │     return;  // ❌ 拒绝无效索引
  │
  ├─ 步骤3: 从gNB配置中提取对应PLMN
  │   req->plmn = rrc->configuration.plmn[idx];
  │   // 例如：idx=1 → plmn[1] = {MCC=208, MNC=93}
  │   // ✅ 正确！使用UE选择的PLMN，不是硬编码plmn[0]
  │
  ├─ 步骤4: 存储UE的serving PLMN
  │   UE->serving_plmn = req->plmn;
  │
  ├─ 步骤5: 提取NAS PDU
  │   req->nas_pdu = create_byte_array(
  │     rrcSetupComplete->dedicatedNAS_Message.size,
  │     rrcSetupComplete->dedicatedNAS_Message.buf);
  │
  ├─ 步骤6: 填充其他信息
  │   req->gNB_ue_ngap_id = UE->rrc_ue_id;
  │   req->establishment_cause = UE->establishment_cause;
  │   req->nr_cell_id = UE->nr_cellid;
  │
  ├─ 步骤7: 日志输出（重要调试信息）
  │   LOG_I(NGAP, "Selected PLMN in the NG Initial UE Message: "
  │              "MCC=%03d MNC=%0*d",
  │              req->plmn.mcc, 
  │              req->plmn.mnc_digit_length, 
  │              req->plmn.mnc);
  │
  └─ 步骤8: 通过ITTI发送消息到NGAP任务
      itti_send_msg_to_task(TASK_NGAP, rrc->module_id, message_p);
```

**uftrace调用树**（从trace提取）:
```
# DURATION     TID     FUNCTION
            [1026442] | rrc_gNB_process_RRCSetupComplete() {
            [1026442] |   rrc_gNB_send_NGAP_NAS_FIRST_REQ() {
            [1026442] |     logRecord_mt() {
   8.176 us [1026442] |       log_output_memory();
   9.516 us [1026442] |     } /* logRecord_mt */
            [1026442] |     itti_send_msg_to_task() {
            [1026442] |       itti_send_msg_to_task_locked() {
  11.257 us [1026442] |         std::vector::insert();
  18.309 us [1026442] |       } /* itti_send_msg_to_task_locked */
  19.540 us [1026442] |     } /* itti_send_msg_to_task */
  33.278 us [1026442] |   } /* rrc_gNB_send_NGAP_NAS_FIRST_REQ */
  33.953 us [1026442] | } /* rrc_gNB_process_RRCSetupComplete */
```

---

### 阶段4: NGAP Initial UE Message发送到AMF (T4)

#### gNB NGAP任务处理
```c
// openair3/NGAP/ngap_gNB_task.c
ngap_gNB_task()
  └─ 接收ITTI消息 NGAP_NAS_FIRST_REQ
     ├─ case NGAP_NAS_FIRST_REQ:
     └─ ngap_gNB_handle_nas_first_req()

// openair3/NGAP/ngap_gNB_nas_procedures.c
ngap_gNB_handle_nas_first_req(instance_t instance, 
                              ngap_nas_first_req_t *nas_first_req)
  ├─ 步骤1: 选择AMF
  │   select_amf(ngap_instance, nas_first_req->establishment_cause, ...)
  │
  ├─ 步骤2: 构造InitialUEMessage ASN.1结构
  │   ngap_generate_initial_ue_message(...)
  │   // 关键字段：
  │   ├─ RAN-UE-NGAP-ID = nas_first_req->gNB_ue_ngap_id
  │   ├─ NAS-PDU = nas_first_req->nas_pdu
  │   ├─ UserLocationInformationNR
  │   │   ├─ NR-CGI (NR Cell Global Identifier)
  │   │   │   ├─ PLMN Identity = nas_first_req->plmn  // 🔑 来自UE选择
  │   │   │   └─ NR Cell ID = nas_first_req->nr_cell_id
  │   │   └─ TAI (Tracking Area Identity)
  │   │       ├─ PLMN Identity = nas_first_req->plmn  // 🔑 再次使用
  │   │       └─ TAC = tracking_area_code
  │   └─ RRCEstablishmentCause = nas_first_req->establishment_cause
  │
  ├─ 步骤3: 编码为二进制
  │   ngap_gNB_encode_pdu()
  │     └─ ngap_gNB_encode_initiating()
  │
  ├─ 步骤4: 存储UE上下文
  │   ngap_store_ue_context()
  │
  └─ 步骤5: 通过SCTP发送给AMF
      ngap_gNB_itti_send_sctp_data_req()
        └─ itti_send_msg_to_task(TASK_SCTP, ...)
```

**uftrace调用树**:
```
# DURATION     TID     FUNCTION
            [1026441] | ngap_gNB_handle_nas_first_req() {
            [1026441] |   select_amf() {
  10.334 us [1026441] |     logRecord_mt();
  12.355 us [1026441] |   } /* select_amf */
            [1026441] |   ngap_gNB_encode_pdu() {
  43.918 us [1026441] |     ngap_gNB_encode_initiating();
  44.715 us [1026441] |   } /* ngap_gNB_encode_pdu */
   6.022 us [1026441] |   ngap_store_ue_context();
            [1026441] |   ngap_gNB_itti_send_sctp_data_req() {
   6.803 us [1026441] |     itti_send_msg_to_task();
   7.533 us [1026441] |   } /* ngap_gNB_itti_send_sctp_data_req */
  95.767 us [1026441] | } /* ngap_gNB_handle_nas_first_req */
```

**关键：PLMN在NGAP消息中的使用**
- NR-CGI中的PLMN：标识小区所属PLMN
- TAI中的PLMN：标识跟踪区域所属PLMN
- **这两个PLMN必须一致**，且必须是AMF支持的PLMN
- 如果PLMN选择错误，AMF会返回"Tracking area not allowed"

---

### 阶段5: AMF处理并响应 (T5)

**AMF侧逻辑**（不在OAI代码中，在Core实现）:
```
AMF收到Initial UE Message
  ├─ 步骤1: 验证TAI (PLMN + TAC)
  │   if (TAI.PLMN not in AMF_supported_PLMNs)
  │     return Registration_Reject("Tracking area not allowed");
  │
  ├─ 步骤2: 解析NAS Registration Request
  │   ├─ 提取SUCI/SUPI (包含UE的PLMN信息)
  │   └─ 验证UE请求的PLMN与TAI中的PLMN是否一致
  │
  ├─ 步骤3: 如果验证通过
  │   ├─ 分配5G-GUTI
  │   ├─ 建立UE上下文
  │   └─ 发送Downlink NAS Transport
  │      └─ 包含NAS Registration Accept
  │
  └─ 步骤4: 如果验证失败
      └─ 发送Downlink NAS Transport
         └─ 包含NAS Registration Reject
            └─ Cause: "Tracking area not allowed"或其他
```

#### gNB接收Downlink NAS
```c
// openair3/NGAP/ngap_gNB_nas_procedures.c
ngap_gNB_handle_downlink_nas_transport()
  └─ 解析NGAP消息，提取NAS PDU
     ├─ itti_send_msg_to_task(TASK_RRC_GNB, ...)
     └─ 消息类型: NGAP_DOWNLINK_NAS

// openair2/RRC/NR/rrc_gNB_NGAP.c
rrc_gNB_process_NGAP_DOWNLINK_NAS(...)
  └─ rrc_forward_ue_nas_message()
     ├─ do_NR_DLInformationTransfer()
     │   └─ 将NAS PDU封装在RRC DL Information Transfer中
     └─ nr_rrc_transfer_protected_rrc_message()
        └─ 通过SRB1发送给UE
```

**uftrace调用树**:
```
# DURATION     TID     FUNCTION
            [1026442] | rrc_gNB_process_NGAP_DOWNLINK_NAS() {
            [1026442] |   rrc_forward_ue_nas_message() {
  10.182 us [1026442] |     logRecord_mt();
  15.448 us [1026442] |     do_NR_DLInformationTransfer();
  11.591 us [1026442] |     nr_rrc_transfer_protected_rrc_message();
  38.982 us [1026442] |   } /* rrc_forward_ue_nas_message */
  40.381 us [1026442] | } /* rrc_gNB_process_NGAP_DOWNLINK_NAS */
```

---

### 阶段6: Initial Context Setup - 安全模式 (T6)

```c
// openair3/NGAP/ngap_gNB_context_management_procedures.c
ngap_gNB_handle_initial_context_setup_req()
  └─ itti_send_msg_to_task(TASK_RRC_GNB, ...)
     └─ 消息: NGAP_INITIAL_CONTEXT_SETUP_REQ

// openair2/RRC/NR/rrc_gNB_NGAP.c:350
rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ(...)
  ├─ set_UE_security_algos()  // 设置加密/完整性保护算法
  ├─ set_UE_security_key()    // 设置Kgnb密钥
  ├─ nr_rrc_pdcp_config_security()  // 配置PDCP安全
  │   └─ nr_derive_key()  // 派生KRRCenc, KRRCint, KUPenc, KUPint
  │      ├─ 调用2次：一次for RRC，一次for UP
  │      └─ 基于Kgnb和安全算法
  └─ rrc_gNB_generate_SecurityModeCommand()
     └─ 发送Security Mode Command给UE
```

**uftrace调用树**（耗时最长的部分）:
```
# DURATION     TID     FUNCTION
            [1026442] | rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ() {
  11.493 us [1026442] |   set_UE_security_algos();
   8.155 us [1026442] |   set_UE_security_key();
            [1026442] |   nr_rrc_pdcp_config_security() {
   1.747 ms [1026442] |     nr_derive_key();  // RRC keys
 776.319 us [1026442] |     nr_derive_key();  // UP keys
   1.317 ms [1026442] |     nr_pdcp_config_set_security();
   3.842 ms [1026442] |   } /* nr_rrc_pdcp_config_security */
  44.685 us [1026442] |   rrc_gNB_generate_SecurityModeCommand();
   3.908 ms [1026442] | } /* rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ */
```

---

### 阶段7: PDU Session Setup - 数据会话建立 (T7)

```c
// openair3/NGAP/ngap_gNB_pdusession_management_procedures.c
ngap_gNB_handle_pdusession_setup_req()
  └─ itti_send_msg_to_task(TASK_RRC_GNB, ...)
     └─ 消息: NGAP_PDUSESSION_SETUP_REQ

// openair2/RRC/NR/rrc_gNB_NGAP.c:800
rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ(...)
  ├─ cp_pdusession_resource_item_to_pdusession()
  │   └─ 提取PDU Session参数（QoS、S-NSSAI等）
  └─ trigger_bearer_setup()
     ├─ add_pduSession()  // 添加PDU Session
     ├─ nr_derive_key()   // 派生UP安全密钥（2次）
     ├─ nr_rrc_add_drb()  // 添加DRB (Data Radio Bearer)
     ├─ get_new_cuup_for_ue()  // 分配CU-UP
     └─ cucp_cuup_bearer_context_setup_direct()
        └─ 通过E1AP配置CU-UP
           └─ 发送RRC Reconfiguration给UE
              ├─ 包含DRB配置
              └─ 包含SDAP配置
```

**uftrace调用树**:
```
# DURATION     TID     FUNCTION
            [1026442] | rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ() {
  18.852 us [1026442] |   logRecord_mt();
  18.889 us [1026442] |   cp_pdusession_resource_item_to_pdusession();
            [1026442] |   trigger_bearer_setup() {
  14.415 us [1026442] |     add_pduSession();
 976.363 us [1026442] |     nr_derive_key();  // UP key 1
 784.125 us [1026442] |     nr_derive_key();  // UP key 2
   7.872 us [1026442] |     nr_rrc_add_drb();
  15.119 us [1026442] |     get_new_cuup_for_ue();
 296.313 us [1026442] |     cucp_cuup_bearer_context_setup_direct();
   2.152 ms [1026442] |   } /* trigger_bearer_setup */
   2.204 ms [1026442] | } /* rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ */
```

---

### 阶段8: 注册完成

UE收到RRC Reconfiguration Complete后：
- DRB建立成功
- 获得IP地址（通过PDU Session）
- 可以开始数据传输

验证：
```bash
# 在UE侧
ping -I oaitun_ue1 10.10.4.2  # ✅ 应该成功
```

---

## 🔍 关键函数调用链总结

### gNB侧完整调用链

```
启动阶段：
main()
  └─ RCconfig_NR_NG()
     └─ RCconfig_NRRRC()
        └─ 读取plmn_list配置

SIB1广播：
nr_schedule_sib1()
  └─ get_SIB1_NR()  // 创建SIB1
     └─ encode_SIB_NR()  // 编码

RRC连接建立：
rrc_gNB_decode_ccch()  // 接收RRC Setup Request
  └─ rrc_gNB_process_RRCSetupRequest()
     └─ rrc_gNB_generate_RRCSetup()

RRC Setup Complete处理（PLMN关键阶段）：
rrc_gNB_decode_dcch()  // 接收RRC Setup Complete
  └─ rrc_gNB_process_RRCSetupComplete()
     └─ rrc_gNB_send_NGAP_NAS_FIRST_REQ()
        ├─ idx = selectedPLMN_Identity - 1
        ├─ req->plmn = plmn[idx]  // 🔑 提取UE选择的PLMN
        └─ itti_send_msg_to_task(TASK_NGAP, ...)

NGAP处理：
ngap_gNB_task()
  └─ ngap_gNB_handle_nas_first_req()
     └─ ngap_generate_initial_ue_message()
        ├─ 填充NR-CGI.PLMN
        ├─ 填充TAI.PLMN
        └─ ngap_gNB_itti_send_sctp_data_req()

接收AMF响应：
ngap_gNB_handle_downlink_nas_transport()
  └─ rrc_gNB_process_NGAP_DOWNLINK_NAS()
     └─ rrc_forward_ue_nas_message()

安全模式：
ngap_gNB_handle_initial_context_setup_req()
  └─ rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ()
     ├─ set_UE_security_algos()
     ├─ nr_rrc_pdcp_config_security()
     └─ rrc_gNB_generate_SecurityModeCommand()

PDU Session建立：
ngap_gNB_handle_pdusession_setup_req()
  └─ rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ()
     └─ trigger_bearer_setup()
        └─ cucp_cuup_bearer_context_setup_direct()
```

---

## 📊 性能数据（从uftrace）

| 阶段 | 函数 | 耗时 |
|------|------|------|
| RRC Setup Complete处理 | `rrc_gNB_process_RRCSetupComplete()` | 33.953 us |
| NGAP Initial UE Msg | `ngap_gNB_handle_nas_first_req()` | 95.767 us |
| Downlink NAS | `rrc_gNB_process_NGAP_DOWNLINK_NAS()` | 40.381 us |
| 安全上下文建立 | `rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ()` | **3.908 ms** |
| ├─ 密钥派生 | `nr_rrc_pdcp_config_security()` | 3.842 ms |
| PDU Session建立 | `rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ()` | **2.204 ms** |
| ├─ Bearer建立 | `trigger_bearer_setup()` | 2.152 ms |

**总耗时约6毫秒**（不含网络传输延迟）

---

## 🐛 Bug修复回顾

### 问题
当gNB广播PLMN列表为`{460-11, 208-93}`时，UE（IMSI PLMN=208-93）注册失败。

### 根本原因
UE在`nr_rrc_process_sib1()`中使用了错误的索引：
```c
// ❌ 错误（使用外层索引i）
rrc->selected_plmn_identity = i + 1;
```

### 修复方案
```c
// ✅ 正确（使用内层索引j）
rrc->selected_plmn_identity = j + 1;
```

### 为什么？
OAI的SIB1 PLMN结构：
```
plmn_IdentityInfoList[0]  // 外层，通常只有一个元素
  └─ plmn_IdentityList
     ├─ [0] = {mcc=460, mnc=11}  // j=0
     └─ [1] = {mcc=208, mnc=93}  // j=1 ← UE应该选择这个
```

UE IMSI=208-93匹配到`plmn_IdentityList[1]`，所以`selectedPLMN_Identity = j+1 = 2` ✅

---

## 📚 参考资料

- **3GPP TS 38.331**: NR RRC Protocol
  - Clause 5.3.3: RRC connection establishment
  - Clause 5.3.4: Initial security activation
- **3GPP TS 38.413**: NG-RAN; NGAP
  - Clause 8.6: Initial UE Message
  - Clause 8.3: Initial Context Setup
- **3GPP TS 24.501**: 5GS NAS Protocol
  - Clause 5.5.1: Registration procedure
- **3GPP TS 38.304**: UE procedures in Idle mode
  - Clause 5.2.3: PLMN selection

---

## 🔧 调试技巧

### 1. 查看PLMN选择日志
```bash
# UE日志
grep -E "\[cyhtest\].*PLMN matched at index" <ue.log>

# gNB日志
grep -E "Selected PLMN in the NG Initial UE Message" <gnb.log>
```

### 2. 使用uftrace追踪
```bash
# 追踪gNB
uftrace replay -d /tmp/trace_dir -F 'rrc_gNB_send_NGAP_NAS_FIRST_REQ' --column-view

# 查看PLMN处理
uftrace replay -d /tmp/trace_dir -F 'ngap_generate_initial_ue_message' -D 3
```

### 3. Wireshark抓包
```bash
# 抓取N2接口（gNB-AMF）
sudo tcpdump -i any sctp -w ngap.pcap

# 在Wireshark中过滤：
# ngap.procedureCode == 15  # InitialUEMessage
```

---

**文档创建时间**: 2025-12-23  
**基于OAI版本**: develop branch (commit 91e7030cf8)  
**3GPP Release**: Rel-15 (向Rel-17迈进)
