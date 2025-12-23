# OAI UE 函数调用树追踪 - 快速开始指南

## ✅ uftrace 已安装并测试成功！

您可以看到树形的函数调用输出，类似于：
```
# DURATION     TID     FUNCTION
  18.372 ms [571470] | main() {
  18.350 ms [571470] |   func_a() {
  15.231 ms [571470] |     func_b() {
  10.113 ms [571470] |       func_c();
```

---

## 🚀 快速开始（3 个简单步骤）

### 步骤 1: 确保使用 Debug 模式编译

```bash
cd ~/openairinterface5g/cmake_targets
./build_oai -g Debug --nrUE -w SIMU
```

**为什么需要 Debug 模式？**
- uftrace 需要函数符号信息
- Debug 模式包含 `-pg` 或 `-finstrument-functions`

---

### 步骤 2: 运行追踪脚本

```bash
cd ~/openairinterface5g

# 快速追踪（只看 RRC/NAS/PDCP）
./cyh_quick_uftrace.sh <你的_ue_config.conf>
```

**运行30秒后自动停止**，或按 Ctrl+C 提前停止。

---

### 步骤 3: 查看结果

脚本会自动显示：
1. 函数调用树
2. 函数统计信息

输出文件：
- `/tmp/oai_rrc_calltree_YYYYMMDD_HHMMSS.txt` - 完整调用树
- `/tmp/ue_run.log` - UE 运行日志

---

## 📊 高级用法

### 完整追踪（所有函数）

```bash
./cyh_uftrace_ue.sh <ue_config.conf>
```

生成多种格式：
- `call_tree.txt` - 文本格式调用树
- `trace.json` - Chrome Trace 格式（可在 chrome://tracing 查看）
- `report.txt` - 函数统计
- `callgraph.dot` - GraphViz 调用图

---

### 自定义过滤

```bash
# 只追踪特定函数及其子调用
./cyh_uftrace_ue.sh <ue_config.conf> -F nr_rrc_ue_generate_rrcSetupRequest

# 限制调用深度为 5
./cyh_uftrace_ue.sh <ue_config.conf> -d 5

# 只显示耗时超过 1ms 的函数
./cyh_uftrace_ue.sh <ue_config.conf> -t 1ms
```

---

## 🎯 典型使用场景

### 场景 1：追踪 UE 注册流程

```bash
cd ~/openairinterface5g

# 1. 启动 gNB（在另一个终端）
cd cmake_targets/ran_build/build
sudo ./nr-softmodem -O <gnb_config> --sa

# 2. 使用 uftrace 追踪 UE
./cyh_quick_uftrace.sh <ue_config>
```

查找关键函数：
```bash
# 查看 RRC Setup 相关
grep "rrcSetup" /tmp/oai_rrc_calltree_*.txt

# 查看认证流程
grep "authentication" /tmp/oai_rrc_calltree_*.txt
```

---

### 场景 2：生成可视化调用图

```bash
# 1. 完整追踪
./cyh_uftrace_ue.sh <ue_config>

# 2. 找到输出目录（例如 /tmp/oai_uftrace_20241222_123456）
OUTPUT_DIR=/tmp/oai_uftrace_YYYYMMDD_HHMMSS

# 3. 生成 PNG 图片
sudo apt install -y graphviz
dot -Tpng $OUTPUT_DIR/callgraph.dot -o $OUTPUT_DIR/callgraph.png

# 4. 查看（如果有 GUI）
xdg-open $OUTPUT_DIR/callgraph.png
```

---

### 场景 3：在 Chrome 中查看时间线

```bash
# 1. 生成 trace.json
./cyh_uftrace_ue.sh <ue_config>

# 2. 下载 trace.json 到本地（如果是远程服务器）
scp node0:/tmp/oai_uftrace_*/trace.json ~/

# 3. 在 Mac 上打开 Chrome
# 访问: chrome://tracing
# 点击 "Load" 加载 trace.json
```

---

## ⚠️ 常见问题

### Q1: "uftrace: /path/to/program is not executable"

**解决方法**：
```bash
# 重新编译为 Debug 模式
cd ~/openairinterface5g/cmake_targets
./build_oai -g Debug --nrUE -w SIMU
```

---

### Q2: 没有捕获到任何函数

**可能原因**：
1. UE 启动失败（检查日志）
2. 追踪时间太短（增加 timeout 时间）
3. 函数过滤太严格（去掉 -F 参数）

**解决方法**：
```bash
# 延长追踪时间到 60 秒
sudo timeout 60s uftrace record -D 8 -d /tmp/test ./nr-uesoftmodem ...
```

---

### Q3: uftrace 太慢，影响实时性

**解决方法**：
- 使用 `-D` 限制深度（例如 `-D 5`）
- 使用 `-F` 只追踪特定函数
- 或者使用 T-Tracer（不影响性能）

---

## 📝 输出示例

### 函数调用树示例
```
# DURATION     TID     FUNCTION
            [  1234] | main() {
   2.123 ms [  1234] |   nr_ue_init() {
   1.500 ms [  1234] |     nr_rrc_ue_init() {
   0.800 ms [  1234] |       nr_rrc_ue_generate_rrcSetupRequest() {
   0.500 ms [  1234] |         encode_rrc_setup_request();
   0.200 ms [  1234] |         send_to_mac();
   0.800 ms [  1234] |       } /* nr_rrc_ue_generate_rrcSetupRequest */
   1.500 ms [  1234] |     } /* nr_rrc_ue_init */
   2.123 ms [  1234] |   } /* nr_ue_init */
            [  1234] | } /* main */
```

### 函数统计示例
```
  Total time   Self time       Calls  Function
  ==========  ==========  ==========  ====================================
    2.123 ms    0.623 ms           1  main
    1.500 ms    0.700 ms           1  nr_ue_init
    0.800 ms    0.300 ms           1  nr_rrc_ue_generate_rrcSetupRequest
    0.500 ms    0.500 ms           1  encode_rrc_setup_request
    0.200 ms    0.200 ms           1  send_to_mac
```

---

## 🎉 总结

**uftrace 是追踪 OAI C 代码的最佳工具**：

| 特性 | 优势 |
|------|------|
| 📊 树形输出 | 清晰显示调用关系 |
| ⏱️ 时间统计 | 显示每个函数的执行时间 |
| 🎨 多种格式 | 支持文本、Chrome、GraphViz |
| ⚡ 性能好 | 比 Valgrind 快很多 |
| 🎯 可过滤 | 只看感兴趣的函数 |

**推荐工作流程**：
1. 使用 `cyh_quick_uftrace.sh` 快速查看 RRC 流程
2. 使用 `cyh_uftrace_ue.sh` 生成完整报告
3. 在 Chrome 中查看时间线（trace.json）
4. 生成调用图（callgraph.png）

**有任何问题随时问我！** 🚀
