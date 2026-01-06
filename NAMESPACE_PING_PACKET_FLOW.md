# Namespace Ping Packet Flow - Complete Analysis

## Network Topology Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│ node0 (gNB + UEs in namespaces)                                     │
│                                                                      │
│  ┌──────────────────┐              ┌──────────────────┐            │
│  │ Namespace ue1    │              │ Namespace ue2    │            │
│  │  v-ue1           │              │  v-ue2           │            │
│  │  10.201.1.1      │              │  10.202.1.1      │            │
│  │  oaitun_ue1      │              │  oaitun_ue2      │            │
│  │  10.60.0.1       │              │  10.60.0.2       │            │
│  └────────┬─────────┘              └────────┬─────────┘            │
│           │ veth pair                       │ veth pair            │
│           │                                 │                      │
│  ┌────────┴─────────┐              ┌───────┴──────────┐           │
│  │ Host (node0)     │              │ Host (node0)     │           │
│  │  v-eth1          │              │  v-eth2          │           │
│  │  10.201.1.100    │              │  10.202.1.100    │           │
│  └──────────────────┘              └──────────────────┘           │
│                                                                     │
│  ┌──────────────────────────────────────────────────────┐         │
│  │ gNB (RFsimulator)                                    │         │
│  │  N3 interface: 10.10.3.1                             │         │
│  └──────────────────────────────────────────────────────┘         │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              │ GTP-U tunnel
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│ node1 (5G Core - CN1)                                               │
│                                                                      │
│  ┌──────────────────────────────────────────────────────┐         │
│  │ UPF (User Plane Function)                            │         │
│  │  N3 interface: 10.10.3.2                             │         │
│  │  N6 interface: 10.10.4.1                             │         │
│  └──────────────────────────────────────────────────────┘         │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│ node3 (DN - Data Network)                                           │
│  IP: 10.10.4.2                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

## NAT Rule Explanation

在 node0 的宿主机上，有如下 iptables NAT 规则：

```bash
iptables -t nat -A POSTROUTING -s 10.201.1.0/24 -o eno1 -j MASQUERADE
iptables -t nat -A POSTROUTING -s 10.202.1.0/24 -o eno1 -j MASQUERADE
```

### NAT 规则详解

**规则作用：**
- `-t nat`: 操作 NAT 表
- `-A POSTROUTING`: 在 POSTROUTING 链添加规则（数据包离开本机前的最后一步）
- `-s 10.201.1.0/24`: 匹配源地址为 namespace 网段的数据包
- `-o eno1`: 匹配从 eno1 网卡出去的数据包
- `-j MASQUERADE`: 执行源地址伪装（SNAT），将源 IP 替换为出接口的 IP

**为什么需要 NAT？**

1. **Namespace IP 不可路由**: 10.201.1.1 是 namespace 内部地址，外部网络不认识
2. **返回路径问题**: 如果不做 NAT，目标主机（如 gNB）不知道如何回应 10.201.1.1
3. **地址转换**: NAT 将 namespace 的私有 IP 转换为宿主机的真实 IP（如 192.168.1.10）
4. **连接跟踪**: NAT 维护连接状态表，能够正确地将返回包转发回 namespace

**MASQUERADE vs SNAT:**
- SNAT: 需要指定固定的源 IP
- MASQUERADE: 自动使用出接口的 IP（适合 DHCP 环境）

## Complete Ping Packet Flow (UE1 → DN)

假设从 namespace ue1 内执行：`ping -I oaitun_ue1 10.10.4.2`

### Part A: 请求包流程（Namespace → DN）

#### Step 1: Namespace 内部发包
```
Source IP: 10.60.0.1 (oaitun_ue1 接口)
Dest IP: 10.10.4.2 (DN)
Protocol: ICMP Echo Request
```
- UE 应用层构造 ping 包
- 源 IP 是 PDU Session IP (10.60.0.1)
- **此时没有经过 namespace 网络层**，直接从 oaitun_ue1 发出

#### Step 2: UE 软件处理
```
Source IP: 10.60.0.1
Dest IP: 10.10.4.2
```
- OAI UE 软件接收到 oaitun_ue1 的数据包
- **不经过 veth 对和 NAT**（因为这是 PDU Session 流量）
- UE 软件将 IP 包封装到 5G NAS 消息中
- 准备通过 RFsimulator 发送给 gNB

#### Step 3: RFsimulator 传输 (UE → gNB)
```
TCP Connection: 10.201.1.1:random_port → 10.201.1.100:4043
Payload: 5G NAS PDU (包含原始 IP 包 10.60.0.1 → 10.10.4.2)
```
- **这里才用到 namespace IP**！
- UE 通过 TCP 连接（源 10.201.1.1）连接到 gNB 的 RFsimulator (10.201.1.100:4043)
- 5G 的用户面数据被封装在 TCP payload 中
- **此时经过 veth pair**：v-ue1 → v-eth1
- **经过 NAT 转换**：10.201.1.1 → 宿主机 IP (例如 192.168.1.10)

#### Step 4: gNB 接收和处理
```
Received from: 192.168.1.10 (经过 NAT 后的地址)
Payload: 提取出 5G NAS PDU
Inner IP: 10.60.0.1 → 10.10.4.2
```
- gNB 从 RFsimulator TCP 连接接收数据
- 解封装出 5G 用户面数据
- 提取出原始 IP 包（10.60.0.1 → 10.10.4.2）
- 准备封装到 GTP-U 隧道

#### Step 5: gNB → UPF (GTP-U 隧道)
```
Outer Header (IP):
  Source IP: 10.10.3.1 (gNB N3 接口)
  Dest IP: 10.10.3.2 (UPF N3 接口)
  Protocol: UDP

GTP-U Header:
  TEID: UE1 的隧道 ID

Inner Packet (用户数据):
  Source IP: 10.60.0.1
  Dest IP: 10.10.4.2
  Protocol: ICMP Echo Request
```
- gNB 将用户 IP 包封装到 GTP-U 隧道
- 外层 IP：10.10.3.1 → 10.10.3.2 (N3 接口通信)
- 内层 IP：保持不变 (10.60.0.1 → 10.10.4.2)

#### Step 6: UPF 解封装
```
接收: Outer IP (10.10.3.1 → 10.10.3.2), GTP-U TEID
解封装后: Inner IP (10.60.0.1 → 10.10.4.2)
```
- UPF 根据 TEID 识别是 UE1 的流量
- 移除 GTP-U 和外层 IP 头
- 得到原始 IP 包

#### Step 7: UPF 转发到 DN
```
Source IP: 10.60.0.1
Dest IP: 10.10.4.2
Interface: N6 (10.10.4.1)
```
- UPF 从 N6 接口转发
- UPF 执行路由查找
- 转发到 DN (10.10.4.2)

#### Step 8: DN 接收
```
Source IP: 10.60.0.1
Dest IP: 10.10.4.2
Protocol: ICMP Echo Request
```
- DN 接收到 ping 请求
- 准备构造 ICMP Echo Reply

### Part B: 响应包流程（DN → Namespace）

#### Step 1: DN 发送响应
```
Source IP: 10.10.4.2
Dest IP: 10.60.0.1
Protocol: ICMP Echo Reply
```

#### Step 2: UPF 接收和封装
```
接收: 10.10.4.2 → 10.60.0.1
查表: 10.60.0.1 属于 UE1，查找对应的 GTP-U 隧道

Outer Header:
  Source IP: 10.10.3.2 (UPF N3)
  Dest IP: 10.10.3.1 (gNB N3)
  Protocol: UDP

GTP-U Header:
  TEID: UE1 下行隧道 ID

Inner Packet:
  Source IP: 10.10.4.2
  Dest IP: 10.60.0.1
  Protocol: ICMP Echo Reply
```

#### Step 3: gNB 接收 GTP-U 包
```
接收: Outer (10.10.3.2 → 10.10.3.1), GTP-U TEID
解封装: Inner (10.10.4.2 → 10.60.0.1)
查表: 该 TEID 对应 UE1 (RFsim 连接 10.201.1.1)
```

#### Step 4: gNB → UE (RFsimulator)
```
TCP Connection: 向 192.168.1.10:random_port 发送 (NAT 连接跟踪)
Payload: 5G NAS PDU (包含 10.10.4.2 → 10.60.0.1)
```
- gNB 将 IP 包封装到 5G NAS PDU
- 通过 RFsimulator TCP 连接发送
- **经过 NAT 逆向转换**：目标地址 192.168.1.10 → 10.201.1.1

#### Step 5: UE 软件接收
```
RFsim 接收: 5G NAS PDU
解封装: IP 包 (10.10.4.2 → 10.60.0.1)
```

#### Step 6: 写入 oaitun_ue1 接口
```
Source IP: 10.10.4.2
Dest IP: 10.60.0.1
Interface: oaitun_ue1
```

#### Step 7: Ping 应用接收
```
ICMP Echo Reply 到达
显示: 64 bytes from 10.10.4.2: icmp_seq=1 ttl=64 time=10 ms
```

## 地址转换汇总表

| 阶段 | 源地址 | 目的地址 | 封装类型 | 说明 |
|------|--------|----------|----------|------|
| Namespace 发出 | 10.60.0.1 | 10.10.4.2 | IP | oaitun_ue1 接口 |
| UE → gNB (RFsim外层) | 10.201.1.1 → 192.168.1.10 (NAT) | 10.201.1.100 | TCP | **经过 NAT** |
| UE → gNB (RFsim内层) | 10.60.0.1 | 10.10.4.2 | 5G NAS PDU | 封装在 TCP 内 |
| gNB → UPF (外层) | 10.10.3.1 | 10.10.3.2 | GTP-U/UDP/IP | N3 接口通信 |
| gNB → UPF (内层) | 10.60.0.1 | 10.10.4.2 | IP | 用户数据包 |
| UPF → DN | 10.60.0.1 | 10.10.4.2 | IP | N6 接口转发 |
| DN → UPF | 10.10.4.2 | 10.60.0.1 | IP | 响应包 |
| UPF → gNB (外层) | 10.10.3.2 | 10.10.3.1 | GTP-U/UDP/IP | N3 接口 |
| UPF → gNB (内层) | 10.10.4.2 | 10.60.0.1 | IP | 用户数据包 |
| gNB → UE (RFsim外层) | 10.201.1.100 | 192.168.1.10 → 10.201.1.1 (逆NAT) | TCP | **经过逆NAT** |
| gNB → UE (RFsim内层) | 10.10.4.2 | 10.60.0.1 | 5G NAS PDU | 封装在 TCP 内 |
| oaitun_ue1 接收 | 10.10.4.2 | 10.60.0.1 | IP | 最终交付 |

## 关键要点总结

### 1. 两层 IP 地址系统
- **Namespace IP (10.201.1.x)**: 用于 RFsimulator 控制平面连接
- **PDU Session IP (10.60.0.x)**: 用于实际数据传输（oaitun_ue1）

### 2. NAT 的作用时机
- **仅在 RFsimulator TCP 连接时生效**
- **不影响用户数据包的 IP 地址** (10.60.0.1 始终不变)
- NAT 转换：10.201.1.1 ↔ 192.168.1.10（宿主机 IP）

### 3. 封装层次
```
┌─────────────────────────────────────────┐
│ TCP (10.201.1.1 → 10.201.1.100)         │  ← 经过 NAT
│  ┌───────────────────────────────────┐  │
│  │ 5G NAS PDU                        │  │
│  │  ┌─────────────────────────────┐  │  │
│  │  │ IP (10.60.0.1 → 10.10.4.2)  │  │  │  ← 用户数据
│  │  │  ┌───────────────────────┐  │  │  │
│  │  │  │ ICMP Echo Request     │  │  │  │
│  │  │  └───────────────────────┘  │  │  │
│  │  └─────────────────────────────┘  │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

### 4. GTP-U 隧道
- **Outer IP**: gNB N3 (10.10.3.1) ↔ UPF N3 (10.10.3.2)
- **Inner IP**: 用户数据 (10.60.0.1 ↔ 10.10.4.2)
- **TEID**: 标识不同 UE 的隧道

### 5. 连接跟踪
- NAT 维护状态表：`10.201.1.1:12345 ↔ 192.168.1.10:12345`
- 返回包能正确转换回 namespace

### 6. 为什么 Namespace IP 不直接通信？
- **RFsimulator 设计**: 需要 TCP 连接到宿主机 IP
- **隔离性**: Namespace 内部网络与外部隔离
- **真实场景模拟**: 类似真实 UE 通过无线信道连接 gNB

## 验证命令

### 查看 NAT 规则
```bash
sudo iptables -t nat -L POSTROUTING -v -n
```

### 查看连接跟踪
```bash
sudo conntrack -L | grep 10.201.1.1
```

### Namespace 内部路由
```bash
sudo ip netns exec ue1 ip route
# 输出应包含:
# default via 10.201.1.100 dev v-ue1
# 10.201.1.0/24 dev v-ue1 proto kernel scope link src 10.201.1.1
```

### 抓包验证
```bash
# 宿主机抓包（看 NAT 前后）
sudo tcpdump -i v-eth1 -n

# Namespace 抓包（看原始地址）
sudo ip netns exec ue1 tcpdump -i v-ue1 -n

# 看 GTP-U 隧道
sudo tcpdump -i eno1 -n 'udp port 2152'
```

## 常见问题

### Q1: 为什么 ping 时源地址是 10.60.0.1 而不是 10.201.1.1？
**A**: 使用 `-I oaitun_ue1` 强制从 PDU Session 接口发出，这是 5G 用户数据流量，不经过 namespace 网络栈。

### Q2: NAT 会修改用户数据包的 IP 吗？
**A**: 不会！NAT 只修改 RFsimulator TCP 连接的外层 IP，用户数据包 (10.60.0.1 → 10.10.4.2) 在整个过程中保持不变。

### Q3: 如果 ping 时不指定 `-I oaitun_ue1` 会怎样？
**A**: 可能从 v-ue1 (10.201.1.1) 发出，这样会经过 namespace 路由和 NAT，但无法通过 5G Core 转发（因为 Core 不认识 10.201.1.1）。

### Q4: 两个 UE 的流量如何区分？
**A**: 
- RFsimulator 层：通过不同的 TCP 连接 (10.201.1.1 vs 10.202.1.1)
- GTP-U 层：通过不同的 TEID
- IP 层：通过不同的 PDU Session IP (10.60.0.1 vs 10.60.0.2)

---

**文档版本**: 1.1  
**最后更新**: 2026-01-04  
**修正历史**: 
- v1.0: UPF N3 接口地址从 10.10.3.3 更正为 10.10.3.2
- v1.1: gNB N3 接口地址从 10.10.3.3 更正为 10.10.3.1
