# Registration Reject 场景分析

## 📊 您遇到的错误

**UE 侧日志**：
```
[NAS] Received Registration reject message too short
```

**Core (AMF) 侧日志**：
```
[AMF][Gmm] Registration Reject[Tracking area not allowed]
```

---

## 🔍 Registration Reject 经历的时间点

### ✅ 会执行的步骤（T1-T8）

| 步骤 | 方向 | 消息/事件 | gNB 函数 | 说明 |
|------|------|----------|---------|------|
| **T1** | UE → gNB | RRC Setup Request | `rrc_gNB_process_initial_ul_rrc_message()` | UE 请求建立 RRC 连接 |
| **T2** | gNB → UE | RRC Setup | `rrc_gNB_generate_RRCSetup()` | gNB 允许建立连接 |
| **T3** | UE → gNB | RRC Setup Complete + NAS Registration Request | `rrc_gNB_process_RRCSetupComplete()` | UE 完成 RRC，发送注册请求 |
| **T4** | gNB → AMF | Initial UE Message (NGAP) | `ngap_generate_initial_ue_message()` | gNB 转发注册请求到 AMF |
| **T5** | AMF 内部 | 检查 TAC | - | ⚠️ **AMF 发现 TAC 不在允许列表** |
| **T6** | AMF → gNB | Downlink NAS Transport (Registration Reject) | `rrc_gNB_process_NGAP_DOWNLINK_NAS()` | AMF 发送拒绝消息 |
| **T7** | gNB → UE | DL RRC Message Transfer | `rrc_gNB_generate_dl_rrc_message()` | gNB 透传拒绝消息 |
| **T8** | UE 侧 | 处理 Registration Reject | - | UE 收到拒绝，记录日志 |

### ❌ 不会执行的步骤（T9-T11）

| 步骤 | 方向 | 消息/事件 | 原因 |
|------|------|----------|------|
| **T9** | AMF → gNB | ~~Authentication Request~~ | ❌ TAC 检查失败，直接拒绝 |
| **T10** | AMF → gNB | ~~Security Mode Command~~ | ❌ 没有认证，不会进入安全流程 |
| **T11** | AMF → gNB | ~~Initial Context Setup~~ | ❌ 注册被拒绝 |
| **T12** | SMF → gNB | ~~PDU Session Setup Request~~ | ❌ 注册失败，不会建立 PDU Session |
| **T13** | gNB → UE | ~~RRC Reconfiguration~~ | ❌ 没有 DRB 需要建立 |

---

## 🆚 正常流程 vs Registration Reject 对比

### **正常注册流程（成功）**

```
UE                    gNB                    AMF                    SMF
 |                     |                      |                      |
 |--RRC Setup Req----->|                      |                      |
 |<---RRC Setup--------|                      |                      |
 |--RRC Setup Cmplt--->|                      |                      |
 |  (NAS Reg Req)      |--Initial UE Msg----->|                      |
 |                     |                      |--检查 TAC (✅ 允许)  |
 |                     |<--Auth Request-------|                      |
 |<--DL NAS (Auth)-----|                      |                      |
 |--UL NAS (Auth Rsp)->|--UL NAS Transfer---->|                      |
 |                     |<--Security Mode------|                      |
 |<--DL NAS (Sec)------|                      |                      |
 |--UL NAS (Sec Rsp)-->|--UL NAS Transfer---->|                      |
 |                     |<--Initial Ctx Setup--|                      |
 |                     |                      |--PDU Session Req---->|
 |                     |<--PDU Session Setup--|<--PDU Session Rsp----|
 |<--RRC Reconfig------|                      |                      |
 |--RRC Reconfig Cmplt>|--UE Ctx Setup Rsp--->|                      |
 |                     |                      |                      |
 [✅ 注册成功，可以使用网络]
```

**gNB Downlink NAS 次数**：3-4 次
1. Authentication Request
2. Security Mode Command
3. Registration Accept (或 Configuration Update)
4. (可选) 其他配置消息

---

### **Registration Reject 流程（失败）**

```
UE                    gNB                    AMF
 |                     |                      |
 |--RRC Setup Req----->|                      |
 |<---RRC Setup--------|                      |
 |--RRC Setup Cmplt--->|                      |
 |  (NAS Reg Req)      |--Initial UE Msg----->|
 |                     |                      |--检查 TAC (❌ 不允许)
 |                     |<--DL NAS (Reject)----|
 |<--Registration------|                      |
 |    Reject           |                      |
 |                     |                      |
 [❌ 注册失败，流程终止]
```

**gNB Downlink NAS 次数**：1 次
1. Registration Reject

**关键差异**：
- ❌ 没有 Authentication Request
- ❌ 没有 Security Mode Command
- ❌ 没有 Registration Accept
- ❌ 没有后续的 PDU Session Setup

---

## 🔍 在 uftrace 中如何识别 Registration Reject？

### **方法 1：查看 Downlink NAS 次数**

```bash
uftrace replay -d /tmp/oai_gnb_trace_xxx \
  --no-libcall -D 30 -t 0us \
  -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | \
  grep -c 'rrc_gNB_process_NGAP_DOWNLINK_NAS'
```

**结果判断**：
- `1-2 次` → 很可能是 Registration Reject
- `3-4 次` → 正常流程（认证 → 安全 → 接受）

---

### **方法 2：查看是否有 PDU Session Setup**

```bash
uftrace replay -d /tmp/oai_gnb_trace_xxx \
  --no-libcall -D 30 -t 0us \
  -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | \
  grep 'PDUSESSION_SETUP'
```

**结果判断**：
- `找到` → 注册成功，进入 PDU Session 建立
- `未找到` → 注册失败（可能是 Reject）

---

### **方法 3：查看时间线**

```bash
# 查看完整的 NAS 交互时间线
uftrace replay -d /tmp/oai_gnb_trace_xxx \
  --no-libcall -D 30 -t 0us \
  -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | \
  grep -E 'rrc_gNB_process_RRCSetupComplete|rrc_gNB_process_NGAP_DOWNLINK_NAS|rrc_gNB_process_NGAP_PDUSESSION' | \
  head -20
```

**正常流程应该看到**：
```
rrc_gNB_process_RRCSetupComplete          ← UE 发送 Registration Request
rrc_gNB_process_NGAP_DOWNLINK_NAS         ← AMF: Authentication Request
rrc_gNB_process_NGAP_DOWNLINK_NAS         ← AMF: Security Mode Command
rrc_gNB_process_NGAP_DOWNLINK_NAS         ← AMF: Registration Accept
rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ ← SMF: PDU Session Setup
```

**Reject 流程只会看到**：
```
rrc_gNB_process_RRCSetupComplete          ← UE 发送 Registration Request
rrc_gNB_process_NGAP_DOWNLINK_NAS         ← AMF: Registration Reject
(后面什么都没有)
```

---

## 🛠️ 如何修复 "Tracking area not allowed" 错误？

### **问题原因**

AMF 配置中的 **TAC (Tracking Area Code)** 不包括 gNB 所在的 TAC。

### **解决方法**

#### **1. 检查 gNB 配置的 TAC**

查看 `gnb.sa.band78.fr1.106PRB.usrpb210.conf`:
```bash
grep -E 'tracking_area_code|tac' gnb.sa.band78.fr1.106PRB.usrpb210.conf
```

应该看到类似：
```
tracking_area_code = 1;
```

#### **2. 检查 AMF 配置的支持 TAC 列表**

在 L25GC+ (或 free5GC) 的 AMF 配置文件（通常是 `amfcfg.yaml`）中：
```yaml
supportTaiList:
  - plmnId:
      mcc: 208
      mnc: 93
    tac: 0x000001    # ← 确保这里包含 gNB 的 TAC (1)
```

#### **3. 修改并重启**

**如果 TAC 不匹配**：
- 方案 A：修改 gNB 的 TAC 为 AMF 支持的值
- 方案 B：在 AMF 配置中添加 gNB 的 TAC

**重启服务**：
```bash
# 重启 AMF
# 重启 gNB
```

---

## 📝 总结

**Registration Reject 场景下**：

✅ **会执行**：
- T1-T4：RRC 建立 + Initial UE Message
- T6-T8：AMF Reject → gNB 转发 → UE 收到

❌ **不会执行**：
- T9-T13：认证、安全、PDU Session（全部跳过）

**识别方法**：
1. gNB trace 中只有 **1 次 Downlink NAS**
2. **没有** PDU Session Setup 相关函数
3. UE/AMF 日志中有明确的 Reject 记录

**修复方法**：
确保 gNB 的 TAC 在 AMF 的 `supportTaiList` 中！
