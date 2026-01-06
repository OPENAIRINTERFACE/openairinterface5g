#!/usr/bin/env bash

# 脚本说明: 修改代码后的快速重新编译脚本
# 用法: ./cyh_rebuild_after_code_change.sh
# 
# 与 cyh_build_oai.sh 的区别:
# - 强制使用 -C 参数清理旧编译
# - 确保代码修改生效

set -e

echo "=========================================="
echo "代码修改后重新编译脚本"
echo "=========================================="

cd ~/openairinterface5g
source oaienv
cd cmake_targets

echo "清理旧的编译缓存..."
rm -rf ran_build/build

echo "=========================================="
echo "开始完全重新编译 OAI RAN..."
echo "目标: gNB, nrUE"
echo "射频: rfsimulator"
echo "附加库: telnetsrv"
echo "构建工具: ninja"
echo "清理模式: 强制清理 (-C)"
echo "=========================================="

./build_oai -C --gNB --nrUE -w SIMU \
  --build-lib telnetsrv \
#   --ninja

echo "=========================================="
echo "编译完成！"
echo "=========================================="
echo "二进制文件位置: ran_build/build/"
echo "  - nr-softmodem (gNB)"
echo "  - nr-uesoftmodem (nrUE)"
echo "  - libtelnetsrv.so (telnet服务器库)"
echo "  - librfsimulator.so (RF模拟器)"
echo "=========================================="
echo ""
echo "下一步操作:"
echo "1. 验证telnet库: ls -la ran_build/build/libtelnetsrv*.so"
echo "2. 启动gNB: sudo ./ran_build/build/nr-softmodem -O ... --rfsim"
echo "3. 启动UE: 参考 MULTI_UE_NAMESPACE_TROUBLESHOOTING.md"
echo "=========================================="
