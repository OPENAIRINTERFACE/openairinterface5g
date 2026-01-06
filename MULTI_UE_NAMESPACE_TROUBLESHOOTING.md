# Multi-UE Namespace 故障排除指南

## 问题1: `libtelnetsrv.so` 库加载失败

### **错误信息**
```
[LOADER] library libtelnetsrv.so is not loaded: libtelnetsrv.so: cannot open shared object file: No such file or directory
[CONFIG] unknown option: --telnetsrv.listenport
[CONFIG] unknown option: 9095
```

### **根本原因**
1. **telnet库未编译**: OAI默认编译可能不包含telnet服务器库
2. **namespace环境变量丢失**: 即使库存在，namespace内的 `LD_LIBRARY_PATH` 也会丢失

### **解决方案A: 不使用telnet功能 (推荐)**

**在namespace内直接运行UE，去掉telnet参数：**

```bash
# 1. 创建namespace
sudo ./tools/scripts/multi-ue.sh -c1

# 2. 进入namespace
sudo ./tools/scripts/multi-ue.sh -o1

# 3. 在namespace内，设置环境变量并启动UE
export LD_LIBRARY_PATH=/users/Yuanhao/openairinterface5g/cmake_targets/ran_build/build:$LD_LIBRARY_PATH

cd ~/openairinterface5g/cmake_targets/ran_build/build

./nr-uesoftmodem \
    -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf \
    -r 106 --numerology 1 --band 78 -C 3619200000 --ssb 516 \
    --rfsim \
    --uicc0.imsi 208930000000003 \
    --rfsimulator.options chanmod \
    --rfsimulator.serveraddr 10.201.1.100
```

**注意**: 
- ✅ **去掉了** `--telnetsrv` 和 `--telnetsrv.listenport 9095` 参数
- ✅ **添加了** `export LD_LIBRARY_PATH` 设置其他必要的库路径

---

### **解决方案B: 使用便捷脚本 (最简单)**

我已经为你创建了两个脚本：

#### **启动UE1:**
```bash
# 1. 创建并进入namespace ue1
sudo ./tools/scripts/multi-ue.sh -c1
sudo ./tools/scripts/multi-ue.sh -o1

# 2. 在namespace内运行脚本
sudo ~/openairinterface5g/cyh_start_ue1_in_namespace.sh
```

#### **启动UE2:**
```bash
# 1. 创建并进入namespace ue2 (新终端)
sudo ./tools/scripts/multi-ue.sh -c2
sudo ./tools/scripts/multi-ue.sh -o2

# 2. 在namespace内运行脚本
sudo ~/openairinterface5g/cyh_start_ue2_in_namespace.sh
```

脚本特性：
- ✅ 自动设置 `LD_LIBRARY_PATH`
- ✅ 自动检查是否在正确的namespace内
- ✅ 如果telnet库不存在，自动跳过telnet参数

---

### **解决方案C: 编译telnet库 (如果需要telnet调试)**

如果你确实需要telnet功能（用于运行时调试和参数调整）：

```bash
# 1. 重新编译，启用telnet支持
cd ~/openairinterface5g/cmake_targets
./build_oai --nrUE -w USRP --build-lib telnetsrv

# 2. 验证库是否存在
ls -la ~/openairinterface5g/cmake_targets/ran_build/build/libtelnetsrv.so
```

然后在namespace内运行时：
```bash
export LD_LIBRARY_PATH=/users/Yuanhao/openairinterface5g/cmake_targets/ran_build/build:$LD_LIBRARY_PATH

./nr-uesoftmodem \
    -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf \
    -r 106 --numerology 1 --band 78 -C 3619200000 --ssb 516 \
    --rfsim \
    --uicc0.imsi 208930000000003 \
    --rfsimulator.options chanmod \
    --rfsimulator.serveraddr 10.201.1.100 \
    --telnetsrv \
    --telnetsrv.listenport 9095
```

---

## 问题2: namespace内环境变量丢失

### **原因**
`ip netns exec ue1 bash` 创建的是**全新的bash环境**，不继承父shell的环境变量。

### **解决方案**

#### **方法1: 每次进入namespace时手动设置**
```bash
sudo ./tools/scripts/multi-ue.sh -o1
export LD_LIBRARY_PATH=/users/Yuanhao/openairinterface5g/cmake_targets/ran_build/build:$LD_LIBRARY_PATH
export PATH=/usr/local/bin:/usr/bin:/bin:$PATH
```

#### **方法2: 修改 multi-ue.sh 脚本自动设置**

编辑 `/users/Yuanhao/openairinterface5g/tools/scripts/multi-ue.sh`，在 `open_namespace()` 函数中：

```bash
open_namespace() {
  [[ $ue_id -ge 1 ]] || die "error: no last UE processed"
  local name="ue$ue_id"
  echo "opening shell in namespace ${name}"
  echo "type 'ip netns exec $name bash' in additional terminals"
  
  # 自动设置环境变量
  ip netns exec $name bash -c "
    export LD_LIBRARY_PATH=/users/Yuanhao/openairinterface5g/cmake_targets/ran_build/build:\$LD_LIBRARY_PATH
    export PS1='[namespace $name] \u@\h:\w\$ '
    bash
  "
}
```

---

## 完整测试流程

### **测试单UE (验证不受影响)**
```bash
# 1. 启动gNB (终端1)
cd ~/openairinterface5g/cmake_targets/ran_build/build
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf \
  --gNBs.[0].min_rxtxtime 6 --rfsim

# 2. 启动单UE (终端2, 在宿主机直接运行)
cd ~/openairinterface5g/cmake_targets/ran_build/build
sudo ./nr-uesoftmodem --rfsim --rfsimulator.serveraddr 127.0.0.1 \
  -r 106 --numerology 1 --band 78 -C 3619200000 --ssb 516 \
  -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf

# ✅ 应该正常工作
```

### **测试多UE (使用namespace)**

#### **准备工作**
```bash
# 确保5G Core正在运行 (node1)
# 确保gNB正在运行 (node0, 终端1)

# 创建namespaces (只需执行一次)
sudo ./tools/scripts/multi-ue.sh -c1
sudo ./tools/scripts/multi-ue.sh -c2
```

#### **启动UE1 (终端2)**
```bash
cd ~/openairinterface5g/tools/scripts
sudo ./multi-ue.sh -o1

# 在namespace ue1内:
export LD_LIBRARY_PATH=/users/Yuanhao/openairinterface5g/cmake_targets/ran_build/build:$LD_LIBRARY_PATH
cd ~/openairinterface5g/cmake_targets/ran_build/build

./nr-uesoftmodem \
    -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf \
    -r 106 --numerology 1 --band 78 -C 3619200000 --ssb 516 \
    --rfsim \
    --uicc0.imsi 208930000000003 \
    --rfsimulator.options chanmod \
    --rfsimulator.serveraddr 10.201.1.100
```

#### **启动UE2 (终端3)**
```bash
cd ~/openairinterface5g/tools/scripts
sudo ./multi-ue.sh -o2

# 在namespace ue2内:
export LD_LIBRARY_PATH=/users/Yuanhao/openairinterface5g/cmake_targets/ran_build/build:$LD_LIBRARY_PATH
cd ~/openairinterface5g/cmake_targets/ran_build/build

./nr-uesoftmodem \
    -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf \
    -r 106 --numerology 1 --band 78 -C 3619200000 --ssb 516 \
    --rfsim \
    --uicc0.imsi 208930000000004 \
    --rfsimulator.options chanmod \
    --rfsimulator.serveraddr 10.202.1.100
```

**注意**: UE2的IMSI改为 `208930000000004`，需要提前在5G Core的数据库中添加此用户。

---

## 验证连接

### **在宿主机检查接口**
```bash
# 应该看到两个UE的TUN接口
ifconfig oaitun_ue1  # UE1的PDU Session IP
ifconfig oaitun_ue2  # UE2的PDU Session IP
```

### **测试连通性**
```bash
# 测试UE1到DN的连通性
ping -I oaitun_ue1 10.10.4.2

# 测试UE2到DN的连通性
ping -I oaitun_ue2 10.10.4.2
```

---

## 常见问题

### Q1: `sudo: unable to resolve host node-0...`
**答**: 这是DNS解析警告，不影响功能，可以忽略。

### Q2: gNB日志显示 "Lost socket"
**答**: 这是UE因参数错误退出导致的，解决参数问题后会消失。

### Q3: 如何清理namespaces?
```bash
sudo ./tools/scripts/multi-ue.sh -d1
sudo ./tools/scripts/multi-ue.sh -d2
```

### Q4: 如何在namespace内查看网络配置?
```bash
# 在namespace内执行
ip addr show        # 查看IP地址
ip route show       # 查看路由表
ping 10.201.1.100   # 测试到宿主机veth的连通性
```

### Q5: 单UE和namespace UE可以同时运行吗?
**答**: 可以！但需要不同的IMSI：
- 单UE: `208930000000003` (在宿主机)
- UE1: `208930000000004` (在namespace ue1)
- UE2: `208930000000005` (在namespace ue2)

---

## 总结

✅ **最简单的解决方案**: 去掉 `--telnetsrv` 参数，添加 `LD_LIBRARY_PATH` 环境变量

✅ **推荐工作流**: 使用我创建的 `cyh_start_ue1_in_namespace.sh` 和 `cyh_start_ue2_in_namespace.sh` 脚本

✅ **telnet功能**: 对于基本测试非必需，仅在需要运行时调试时才编译和使用
