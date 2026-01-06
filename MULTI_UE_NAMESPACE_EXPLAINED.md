# multi-ue.sh 脚本详解
## 如何在同一主机上运行多个OAI nrUE

---

## 🎯 核心问题

**为什么需要 namespace？**

在同一台主机上运行多个UE时，存在以下冲突：
1. **网络接口冲突**：所有UE默认都会创建同名的网络接口（如 `oaitun_ue1`）
2. **IP地址冲突**：所有UE可能获得相同的IP地址
3. **RFsimulator地址冲突**：默认都连接到同一个地址

**解决方案：Linux Network Namespace**

Network Namespace 可以为每个UE创建**完全隔离的网络环境**，就像在不同的虚拟机中运行一样。

---

## 📖 命令执行流程分析

### 步骤1: 创建UE1的namespace

```bash
sudo ./multi-ue.sh -c1
```

这个命令会调用 `create_namespace(1)`，执行以下操作：

#### 1.1 创建Network Namespace
```bash
ip netns add ue1
```

**作用**：创建一个名为 `ue1` 的隔离网络命名空间

**效果**：
- 新的网络栈（独立的路由表、防火墙规则、网络接口）
- 完全独立于主机的网络环境
- 类似于创建了一个轻量级的"虚拟网络环境"

#### 1.2 创建虚拟网络设备对（veth pair）
```bash
ip link add v-eth1 type veth peer name v-ue1
```

**作用**：创建一对虚拟以太网接口

**可视化**：
```
主机网络环境                  namespace ue1
┌──────────────┐            ┌──────────────┐
│              │            │              │
│   v-eth1 ●───┼────────────┼───● v-ue1    │
│              │  (虚拟网线) │              │
└──────────────┘            └──────────────┘
```

**工作原理**：
- `v-eth1` 和 `v-ue1` 像一根虚拟网线的两端
- 从 `v-eth1` 发送的数据会从 `v-ue1` 接收，反之亦然
- 类似于两台机器之间用网线连接

#### 1.3 将 v-ue1 移入 namespace
```bash
ip link set v-ue1 netns ue1
```

**作用**：将 `v-ue1` 接口移动到 `ue1` namespace 中

**结果**：
```
主机网络环境                  namespace ue1
┌──────────────┐            ┌──────────────┐
│              │            │              │
│   v-eth1 ●───┼────────────┼───● v-ue1    │
│              │            │   (现在在这里) │
└──────────────┘            └──────────────┘
```

#### 1.4 配置主机侧的IP地址
```bash
BASE_IP=$((200+ue_id))  # ue_id=1, 所以 BASE_IP=201
ip addr add 10.201.1.100/24 dev v-eth1
ip link set v-eth1 up
```

**作用**：
- 计算基础IP：`BASE_IP = 200 + 1 = 201`
- 给主机侧的 `v-eth1` 分配IP：`10.201.1.100/24`
- 激活接口

**IP计算逻辑**：
```
UE1: BASE_IP = 201 → 10.201.1.100/24 (主机侧)
UE2: BASE_IP = 202 → 10.202.1.100/24 (主机侧)
UE3: BASE_IP = 203 → 10.203.1.100/24 (主机侧)
...
```

**为什么每个UE用不同的子网？**
- 避免路由冲突
- 每个UE有独立的 `/24` 网络段

#### 1.5 配置NAT和转发规则
```bash
iptables -t nat -A POSTROUTING -s 10.201.1.0/255.255.255.0 -o lo -j MASQUERADE
iptables -A FORWARD -i lo -o v-eth1 -j ACCEPT
iptables -A FORWARD -o lo -i v-eth1 -j ACCEPT
```

**作用**：允许namespace内的流量访问外部网络

**详细解释**：

**第1条规则（MASQUERADE）**：
```bash
iptables -t nat -A POSTROUTING -s 10.201.1.0/255.255.255.0 -o lo -j MASQUERADE
```
- 来自 `10.201.1.0/24` 网段的数据包
- 经过 `lo` (loopback) 接口时
- 进行源地址转换（SNAT）
- 将源IP改为主机IP

**第2-3条规则（FORWARD）**：
```bash
iptables -A FORWARD -i lo -o v-eth1 -j ACCEPT
iptables -A FORWARD -o lo -i v-eth1 -j ACCEPT
```
- 允许 `lo` ↔ `v-eth1` 之间的双向转发
- 使namespace能与主机通信

**流量路径示例**：
```
namespace ue1        主机           gNB (127.0.0.1:4043)
10.201.1.1    →    10.201.1.100   →   127.0.0.1:4043
   (UE进程)      (v-eth1, NAT)        (RFsimulator)
```

#### 1.6 配置namespace内的网络
```bash
ip netns exec ue1 ip link set dev lo up
ip netns exec ue1 ip addr add 10.201.1.1/24 dev v-ue1
ip netns exec ue1 ip link set v-ue1 up
```

**作用**：在 `ue1` namespace 内部配置网络

**`ip netns exec ue1 <命令>`**：
- 在 `ue1` namespace 中执行命令
- 相当于"进入"这个隔离环境

**具体操作**：
1. 启动loopback接口 (`lo`)
2. 给 `v-ue1` 分配IP：`10.201.1.1/24`
3. 激活 `v-ue1` 接口

**最终网络拓扑**：
```
┌─────────────────────────────────────────────────────────────┐
│ 主机网络环境                                                  │
│                                                               │
│  v-eth1: 10.201.1.100/24 ●───────────┐                      │
│                                       │                      │
│  lo: 127.0.0.1 (gNB RFsim监听)        │                      │
│                                       │                      │
│  ┌────────────────────────────────────┼──────────────────┐  │
│  │ namespace ue1                      │                  │  │
│  │                                    │                  │  │
│  │  ● v-ue1: 10.201.1.1/24            │                  │  │
│  │  (连接到 v-eth1)                   │                  │  │
│  │                                    │                  │  │
│  │  lo: 127.0.0.1                     │                  │  │
│  │                                    │                  │  │
│  │  [UE1进程运行在这里]                │                  │  │
│  └────────────────────────────────────┘                  │  │
└─────────────────────────────────────────────────────────────┘
```

---

### 步骤2: 进入UE1的namespace

```bash
sudo ./multi-ue.sh -o1
```

这个命令会调用 `open_namespace()`：

```bash
open_namespace() {
  [[ $ue_id -ge 1 ]] || die "error: no last UE processed"
  local name="ue$ue_id"
  echo "opening shell in namespace ${name}"
  echo "type 'ip netns exec $name bash' in additional terminals"
  ip netns exec $name bash
}
```

**作用**：
- 在 `ue1` namespace 中启动一个新的bash shell
- 在这个shell中执行的所有命令都在隔离的网络环境中
- 就像SSH到一台独立的机器

**进入后的环境**：
```bash
# 你现在在 namespace ue1 中
# 查看网络接口
ip addr show
  lo: 127.0.0.1
  v-ue1: 10.201.1.1/24

# 查看路由表（独立的路由表）
ip route show

# ping测试（通过NAT访问主机）
ping 10.201.1.100  # 可以ping通主机侧的v-eth1
```

---

### 步骤3: 在namespace中启动UE1

```bash
sudo ./nr-uesoftmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf \
  -r 106 --numerology 1 --band 78 -C 3619200000 \
  --rfsim \
  --uicc0.imsi 001010000000001 \
  --rfsimulator.options chanmod \
  --rfsimulator.serveraddr 10.201.1.100 \  # 🔑 关键：连接到主机侧的地址
  --telnetsrv \
  --telnetsrv.listenport 9095
```

**关键参数解释**：

#### `--rfsimulator.serveraddr 10.201.1.100`
**这是整个机制的核心！**

**工作原理**：
```
1. UE1进程在namespace ue1中运行
2. UE1尝试连接RFsimulator server地址: 10.201.1.100
3. 这个地址是主机侧的v-eth1接口
4. 数据包路径：
   v-ue1 (10.201.1.1) 
     → v-eth1 (10.201.1.100) 
     → iptables NAT 
     → lo (127.0.0.1) 
     → gNB RFsimulator (监听127.0.0.1:4043)
```

**为什么不直接用 `127.0.0.1`？**
- 因为在namespace中，`127.0.0.1` 是namespace自己的loopback
- 不是主机的loopback
- 必须通过veth pair连接到主机网络

#### `--uicc0.imsi 001010000000001`
- 每个UE必须有不同的IMSI
- 在5GC的数据库中预先配置

#### `--telnetsrv.listenport 9095`
- 每个UE在不同端口开启telnet服务器
- 用于运行时配置和监控

---

### 步骤4: 创建并运行UE2

**创建UE2的namespace**：
```bash
sudo ./multi-ue.sh -c2
```

执行相同的流程，但参数不同：
```bash
BASE_IP = 200 + 2 = 202
主机侧：v-eth2: 10.202.1.100/24
namespace侧：v-ue2: 10.202.1.1/24
```

**进入UE2的namespace**：
```bash
sudo ./multi-ue.sh -o2
```

**启动UE2**：
```bash
sudo ./nr-uesoftmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf \
  -r 106 --numerology 1 --band 78 -C 3619200000 \
  --rfsim \
  --uicc0.imsi 001010000000002 \  # 不同的IMSI
  --rfsimulator.options chanmod \
  --rfsimulator.serveraddr 10.202.1.100 \  # UE2的主机侧地址
  --telnetsrv \
  --telnetsrv.listenport 9096  # 不同的端口
```

---

## 🔍 完整的网络拓扑

```
┌──────────────────────────────────────────────────────────────────────┐
│ 主机网络环境 (node0)                                                  │
│                                                                        │
│  gNB进程 (RFsimulator)                                                │
│    监听: 127.0.0.1:4043                                               │
│         ↑                                                             │
│         │ (通过lo和NAT)                                               │
│         │                                                             │
│  ┌──────┴─────────────────┬─────────────────────────┐                │
│  │                        │                         │                │
│  v-eth1: 10.201.1.100/24  v-eth2: 10.202.1.100/24   ...              │
│       ↓                         ↓                                     │
│       │                         │                                     │
│  ┌────┼────────────────┐  ┌────┼────────────────┐                   │
│  │    │ namespace ue1  │  │    │ namespace ue2  │                   │
│  │    ↓                │  │    ↓                │                   │
│  │ v-ue1: 10.201.1.1  │  │ v-ue2: 10.202.1.1  │                   │
│  │                     │  │                     │                   │
│  │ [UE1进程]           │  │ [UE2进程]           │                   │
│  │ IMSI: ...001        │  │ IMSI: ...002        │                   │
│  │ telnet: 9095        │  │ telnet: 9096        │                   │
│  │ oaitun_ue1 →5GC IP  │  │ oaitun_ue1 →5GC IP  │  (名称可以相同!)  │
│  └─────────────────────┘  └─────────────────────┘                   │
│                                                                        │
└──────────────────────────────────────────────────────────────────────┘

通过N3接口到5GC：
  UE1和UE2都通过gNB连接到AMF/UPF
  每个UE获得不同的PDU Session IP（从5GC分配）
```

---

## 💡 关键技术点总结

### 1. Network Namespace 隔离
- **每个UE有独立的网络栈**
- 接口名可以重复（都叫`oaitun_ue1`）
- IP地址可以重复（都从5GC获得`12.1.1.x`）
- 互不干扰

### 2. veth pair 连接
- 虚拟网线连接namespace和主机
- 高效的进程间通信
- 零拷贝传输

### 3. NAT 地址转换
- namespace内部访问主机资源
- 通过不同的子网区分不同UE
- iptables规则实现转发

### 4. RFsimulator 寻址
- UE不直接连接`127.0.0.1`（namespace隔离）
- 通过veth pair地址`10.20X.1.100`连接
- NAT转发到主机的`127.0.0.1:4043`

---

## 🎯 为什么这个方案有效？

### 问题1：多个UE如何同时连接一个gNB？
**答案**：通过不同的虚拟网络接口
- UE1: `v-ue1` (10.201.1.1) → `v-eth1` (10.201.1.100) → gNB
- UE2: `v-ue2` (10.202.1.1) → `v-eth2` (10.202.1.100) → gNB

### 问题2：接口名冲突怎么办？
**答案**：namespace隔离
- UE1的`oaitun_ue1`在namespace `ue1`中
- UE2的`oaitun_ue1`在namespace `ue2`中
- 两者不在同一个网络栈，不冲突

### 问题3：IMSI如何区分？
**答案**：命令行参数
- `--uicc0.imsi 001010000000001` (UE1)
- `--uicc0.imsi 001010000000002` (UE2)
- 5GC通过IMSI识别不同UE

### 问题4：telnet端口冲突怎么办？
**答案**：不同端口
- UE1: `--telnetsrv.listenport 9095`
- UE2: `--telnetsrv.listenport 9096`

---

## 🔧 实用命令

### 查看所有namespace
```bash
ip netns list
  ue2
  ue1
```

### 在namespace中执行单条命令
```bash
# 查看UE1的IP地址
sudo ip netns exec ue1 ip addr

# 查看UE1的路由表
sudo ip netns exec ue1 ip route

# 在UE1中ping 5GC
sudo ip netns exec ue1 ping -I oaitun_ue1 192.168.70.135
```

### 查看UE1的进程
```bash
sudo ip netns exec ue1 ps aux | grep nr-uesoftmodem
```

### 查看UE1的网络连接
```bash
sudo ip netns exec ue1 netstat -tuln
```

### 从主机访问UE的telnet
```bash
# UE1的telnet (在主机上执行)
telnet localhost 9095

# UE2的telnet
telnet localhost 9096
```

### 删除namespace
```bash
sudo ./multi-ue.sh -d1  # 删除ue1
sudo ./multi-ue.sh -d2  # 删除ue2
```

---

## 📊 数据流向示例

### UE1注册流程数据包路径

```
1. UE1发送RRC Setup Request:
   UE1进程 (namespace ue1)
     → v-ue1 (10.201.1.1)
     → v-eth1 (10.201.1.100)
     → iptables NAT (源地址转换)
     → lo (127.0.0.1)
     → gNB RFsimulator (127.0.0.1:4043)

2. gNB响应RRC Setup:
   gNB (127.0.0.1:4043)
     → lo
     → iptables (反向NAT)
     → v-eth1
     → v-ue1
     → UE1进程 (namespace ue1)

3. UE1与5GC通信（PDU Session建立后）:
   UE1应用
     → oaitun_ue1 (5GC分配的IP，如12.1.1.2)
     → gNB (通过RFsim)
     → N3接口
     → UPF
     → 外部网络 (如ping 192.168.70.135)
```

---

## ⚠️ 注意事项

### 1. IMSI必须预先配置
在5GC的MySQL数据库中添加每个UE的IMSI：
```sql
INSERT INTO AuthenticationSubscription (ueid, ...) 
VALUES ('001010000000001', ...);
INSERT INTO AuthenticationSubscription (ueid, ...) 
VALUES ('001010000000002', ...);
```

### 2. 资源限制
- 每个UE消耗CPU/内存
- 建议测试不超过10个UE（取决于硬件）

### 3. namespace不会自动清理
- 手动删除：`sudo ./multi-ue.sh -d<ID>`
- 或重启系统自动清理

### 4. 需要root权限
- network namespace操作需要root
- 所有命令都用`sudo`执行

---

## 🎓 总结

`multi-ue.sh` 通过以下技术实现多UE：

1. **Linux Network Namespace**: 隔离网络环境
2. **veth pair**: 连接namespace和主机
3. **不同的IP子网**: 每个UE独立的网络段
4. **NAT转发**: 使namespace能访问主机资源
5. **不同的参数**: IMSI、telnet端口等

**核心思想**：
- 每个UE = 一个独立的"虚拟网络环境"
- 通过虚拟网线连接到主机
- 所有UE共享同一个gNB（通过不同的虚拟连接）

**为什么有效**：
- ✅ 完全隔离：每个UE有独立的网络栈
- ✅ 高效：轻量级，比虚拟机快
- ✅ 灵活：可以轻松添加/删除UE
- ✅ 真实：每个UE都是完整的OAI nrUE进程

这就是为什么执行 `-c1 -o1` 后就能运行UE1，执行 `-c2 -o2` 后就能运行UE2的原因！
