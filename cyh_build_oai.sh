#!/usr/bin/env bash

# 脚本说明: OAI RAN编译脚本（用于rfsimulator模式）
# 包含: gNB, nrUE, telnet服务器支持
# 用法: ./cyh_build_oai.sh [clean]
#       参数 clean: 清除旧编译，完全重新构建

set -e

cd ~/openairinterface5g
source oaienv
cd cmake_targets

# 检查是否需要清理编译
if [ "$1" == "clean" ]; then
    echo "=========================================="
    echo "清理旧的编译文件..."
    echo "=========================================="
    sudo rm -rf ran_build/build
    CLEAN_FLAG="-c"
else
    CLEAN_FLAG=""
fi

# 编译OAI (gNB + nrUE + telnet + ninja)
echo "=========================================="
echo "开始编译 OAI RAN..."
echo "目标: gNB, nrUE"
echo "射频: rfsimulator"
echo "附加库: telnetsrv"
echo "构建工具: ninja"
echo "=========================================="

./build_oai --gNB --nrUE -w SIMU \
  --build-lib telnetsrv \
#   --ninja \
  $CLEAN_FLAG

echo "=========================================="
echo "编译完成！"
echo "二进制文件位置: ran_build/build/"
echo "  - nr-softmodem (gNB)"
echo "  - nr-uesoftmodem (nrUE)"
echo "  - libtelnetsrv.so (telnet库)"
echo "=========================================="

