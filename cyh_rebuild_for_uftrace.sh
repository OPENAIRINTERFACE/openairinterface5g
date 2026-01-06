#!/usr/bin/env bash
# 重新编译 OAI，启用 uftrace 支持
# 关键：加上 -pg 和 -g 标志

set -e

cd ~/openairinterface5g
source oaienv
cd cmake_targets

echo "================================================"
echo "清理旧的编译文件（避免混入旧 object）"
echo "================================================"
rm -rf ran_build/build

echo ""
echo "================================================"
echo "开始编译（启用函数插桩，支持 uftrace）"
echo "================================================"
./build_oai --gNB --nrUE -w SIMU \
  -g Debug \
  --cmake-opt "-DCMAKE_C_FLAGS=\"-g -O1 -fno-omit-frame-pointer\"" \
  --cmake-opt "-DCMAKE_CXX_FLAGS=\"-g -O1 -fno-omit-frame-pointer\""

echo ""
echo "================================================"
echo "✅ 编译完成！"
echo "================================================"
echo "现在可以使用 uftrace 追踪 nr-softmodem 和 nr-uesoftmodem"
echo ""
echo "⚠️  注意："
echo "   1. 带 -pg 的版本会比平时慢，只用于调试"
echo "   2. 想恢复正常版本，重新运行 ./build_oai --gNB --nrUE -w SIMU 即可"
echo ""
