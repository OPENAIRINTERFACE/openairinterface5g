#!/usr/bin/env bash
# 快速启动 UE（自动设置库路径）

set -e

cd ~/openairinterface5g/cmake_targets/ran_build/build

echo "================================================"
echo "启动 NR UE"
echo "================================================"
echo "⚠️  确保 gNB 已经在运行！"
echo ""

# 设置库路径
export LD_LIBRARY_PATH=$(pwd):$LD_LIBRARY_PATH

# 启动 UE
sudo -E ./nr-uesoftmodem \
  --rfsim \
  --rfsimulator.serveraddr 127.0.0.1 \
  -r 106 \
  --numerology 1 \
  --band 78 \
  -C 3619200000 \
  --ssb 516 \
  -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf
