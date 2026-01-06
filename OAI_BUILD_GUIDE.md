# OAI 编译完整指南

## 📋 当前项目编译状态

### **已编译内容**
```bash
./build_oai -I --gNB --nrUE -w SIMU
```

✅ gNB可执行文件 (`nr-softmodem`)  
✅ nrUE可执行文件 (`nr-uesoftmodem`)  
✅ rfsimulator库 (`librfsimulator.so`)  
❌ telnet服务器库 (`libtelnetsrv.so`) - **未编译**  
❌ nrscope示波器库 - **未编译**  
❌ UHD驱动 - **不需要（使用rfsimulator）**

---

## 🎯 编译选项详解

### **1. telnet库 (telnetsrv)**

#### **作用**
- 提供运行时telnet服务器
- 可以在UE/gNB运行时通过telnet连接调试
- 查看统计信息、修改参数、触发特定动作

#### **是否必需**
- ❌ **非必需**: UE注册、数据传输不依赖telnet
- ✅ **推荐**: 多UE教程示例中使用，便于调试

#### **编译方法**
```bash
./build_oai --gNB --nrUE -w SIMU --build-lib telnetsrv
```

#### **使用方法**
```bash
# 启动时添加telnet参数
./nr-uesoftmodem ... --telnetsrv --telnetsrv.listenport 9090

# 连接telnet (另一个终端)
telnet localhost 9090
```

#### **相关文档**
- [telnetusage.md](common/utils/telnetsrv/DOC/telnetusage.md)

---

### **2. UHD驱动**

#### **作用**
- Ettus USRP硬件的驱动库
- 支持 USRP B210, N300, X300等射频设备

#### **是否需要**
- ❌ **你不需要**: 使用rfsimulator (`-w SIMU`)
- ✅ **需要的场景**: 使用真实USRP硬件 (`-w USRP`)

#### **安装方法（如果需要）**
```bash
# 方法1: 从PPA安装（Ubuntu）
./build_oai -I -w USRP

# 方法2: 从源码安装特定版本
export BUILD_UHD_FROM_SOURCE=True
export UHD_VERSION=4.6.0.0
./build_oai -I -w USRP
```

#### **相关文档**
- [NR_SA_Tutorial_OAI_nrUE.md](doc/NR_SA_Tutorial_OAI_nrUE.md)
- UHD官方: https://github.com/EttusResearch/uhd

---

### **3. ninja构建工具**

#### **作用**
- 替代传统`make`的构建系统
- 更快的并行编译

#### **推荐**
✅ **强烈推荐使用**

#### **对比**
```bash
# 使用make (慢)
./build_oai --gNB --nrUE -w SIMU
# 编译时间: ~15分钟 (8核CPU)

# 使用ninja (快)
./build_oai --gNB --nrUE -w SIMU --ninja
# 编译时间: ~8分钟 (8核CPU)
```

#### **安装ninja**
```bash
sudo apt install -y ninja-build
```

---

### **4. nrscope示波器库**

#### **作用**
- 图形化显示PHY层信号
- 实时查看频域/时域波形
- 便于调试物理层问题

#### **是否需要**
- ❌ **一般不需要**: 基本功能测试不需要
- ✅ **需要的场景**: PHY层调试、演示、学术研究

#### **依赖**
```bash
sudo apt install -y libforms-dev libforms-bin
```

#### **编译方法**
```bash
./build_oai --gNB --nrUE -w SIMU --build-lib nrscope
```

#### **使用方法**
```bash
# 启动UE时启用scope
./nr-uesoftmodem ... --nrUE-phy-scope
```

---

### **5. `-C` 清理参数**

#### **作用**
- **Clean**: 删除旧的编译缓存
- 强制完全重新编译

#### **何时使用**
| 场景 | 是否使用-C | 原因 |
|------|-----------|------|
| 首次编译 | ❌ | 没有旧缓存 |
| 日常重复编译 | ❌ | 浪费时间 |
| 修改源代码后 | ✅ | 确保修改生效 |
| 切换编译选项 | ✅ | 避免冲突 |
| 编译出错 | ✅ | 清除损坏的缓存 |
| 更新git代码后 | ✅ | 确保干净编译 |

#### **示例**
```bash
# 修改了 rrc_UE.c 中的PLMN选择逻辑
vim openair2/RRC/NR_UE/rrc_UE.c
./build_oai -C --gNB --nrUE -w SIMU --ninja  # ✅ 使用-C
```

---

## 🔧 推荐编译命令

### **方案A: 最小化编译（满足多UE测试）**

```bash
cd ~/openairinterface5g
source oaienv
cd cmake_targets

./build_oai --gNB --nrUE -w SIMU \
  --build-lib telnetsrv \
  --ninja
```

**编译时间**: ~8分钟  
**包含**: gNB, nrUE, rfsimulator, telnet库  
**适用**: 多UE测试、基本功能验证

---

### **方案B: 完整开发编译（推荐）**

```bash
cd ~/openairinterface5g
source oaienv
cd cmake_targets

# 首先安装nrscope依赖（可选）
sudo apt install -y libforms-dev libforms-bin

# 编译
./build_oai -C --gNB --nrUE -w SIMU \
  --build-lib "telnetsrv nrscope" \
  --ninja
```

**编译时间**: ~10分钟  
**包含**: gNB, nrUE, rfsimulator, telnet库, nrscope  
**适用**: 开发调试、PHY层分析

---

### **方案C: 修改代码后重新编译**

```bash
# 使用我创建的脚本
./cyh_rebuild_after_code_change.sh
```

**等价于**:
```bash
cd ~/openairinterface5g
source oaienv
cd cmake_targets

./build_oai -C --gNB --nrUE -w SIMU \
  --build-lib telnetsrv \
  --ninja
```

---

## 📝 实际操作步骤

### **步骤1: 编译telnet库**

由于你之前没有编译telnet库，现在补充编译：

```bash
cd ~/openairinterface5g
source oaienv
cd cmake_targets

# 方法1: 只编译telnet库（快速）
./build_oai --build-lib telnetsrv

# 方法2: 完全重新编译，包含telnet（推荐）
./build_oai -C --gNB --nrUE -w SIMU \
  --build-lib telnetsrv \
  --ninja
```

**验证编译结果**:
```bash
ls -la ~/openairinterface5g/cmake_targets/ran_build/build/libtelnetsrv*.so

# 应该看到:
# libtelnetsrv.so
# libtelnetsrv_5GUE.so
# libtelnetsrv_gnb.so
```

---

### **步骤2: 测试多UE（使用telnet）**

```bash
# 1. 创建namespace
sudo ./tools/scripts/multi-ue.sh -c1

# 2. 进入namespace
sudo ./tools/scripts/multi-ue.sh -o1

# 3. 设置环境变量并启动UE
export LD_LIBRARY_PATH=/users/Yuanhao/openairinterface5g/cmake_targets/ran_build/build:$LD_LIBRARY_PATH

cd ~/openairinterface5g/cmake_targets/ran_build/build

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

## 🔍 常见问题

### Q1: 编译时报错 "ninja: command not found"
```bash
sudo apt install -y ninja-build
```

### Q2: 编译时报错 "libforms not found" (如果编译nrscope)
```bash
sudo apt install -y libforms-dev libforms-bin
```

### Q3: 如何验证telnet库是否正确编译?
```bash
# 方法1: 查看文件
ls -la ~/openairinterface5g/cmake_targets/ran_build/build/libtelnetsrv*.so

# 方法2: 运行UE时观察日志
./nr-uesoftmodem ... --telnetsrv
# 应该看到: [LOADER] library libtelnetsrv.so successfully loaded
```

### Q4: 我修改了代码，应该如何重新编译?
```bash
# 推荐: 使用-C强制完全重新编译
./cyh_rebuild_after_code_change.sh

# 或者手动:
cd ~/openairinterface5g/cmake_targets
./build_oai -C --gNB --nrUE -w SIMU --build-lib telnetsrv --ninja
```

### Q5: 如何同时编译多个库?
```bash
# 空格分隔多个库名
./build_oai --gNB --nrUE -w SIMU \
  --build-lib "telnetsrv nrscope ldpc" \
  --ninja
```

### Q6: 如何查看所有可用的编译选项?
```bash
cd ~/openairinterface5g/cmake_targets
./build_oai -h
```

---

## 📚 参考文档

- [BUILD.md](doc/BUILD.md) - OAI官方编译文档
- [NR_SA_Tutorial_OAI_nrUE.md](doc/NR_SA_Tutorial_OAI_nrUE.md) - 单UE部署教程
- [NR_SA_Tutorial_OAI_multi_UE.md](doc/NR_SA_Tutorial_OAI_multi_UE.md) - 多UE部署教程
- [telnetusage.md](common/utils/telnetsrv/DOC/telnetusage.md) - Telnet使用文档

---

## 🎯 总结

### **你的情况**

| 项目 | 当前状态 | 是否需要 | 建议操作 |
|------|---------|----------|---------|
| gNB/nrUE | ✅ 已编译 | ✅ 必需 | 保持 |
| rfsimulator | ✅ 已编译 | ✅ 必需 | 保持 |
| telnet库 | ❌ 未编译 | ⚠️ 推荐 | **补充编译** |
| UHD驱动 | ❌ 未安装 | ❌ 不需要 | 跳过 |
| nrscope | ❌ 未编译 | ❌ 可选 | 可选 |
| ninja | ✅ 可用 | ✅ 推荐 | 使用 |

### **推荐操作**

```bash
# 立即执行: 编译telnet库
cd ~/openairinterface5g
source oaienv
cd cmake_targets
./build_oai -C --gNB --nrUE -w SIMU --build-lib telnetsrv --ninja
```

编译完成后，你的多UE部署将完全支持教程中的所有功能！
