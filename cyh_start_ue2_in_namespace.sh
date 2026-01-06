#!/bin/bash

# 脚本说明：在namespace ue2中启动UE2
# 用法: sudo ./cyh_start_ue2_in_namespace.sh

echo "=========================================="
echo "Starting UE2 in namespace ue2"
echo "=========================================="

# 检查是否在namespace内
if [ -z "$(ip netns identify)" ]; then
    echo "错误: 请先执行 sudo ./tools/scripts/multi-ue.sh -o2 进入namespace ue2"
    exit 1
fi

# 设置必要的环境变量
export LD_LIBRARY_PATH=/usr/local/lib:/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH

# 检查动态库是否存在
if [ ! -f "/usr/local/lib/libtelnetsrv.so" ]; then
    echo "警告: libtelnetsrv.so 未找到，telnet功能将不可用"
    echo "继续启动UE (不使用telnet)..."
    
    # 不使用telnet参数启动
    cd ~/openairinterface5g/cmake_targets/ran_build/build
    ./nr-uesoftmodem \
        -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf \
        -r 106 --numerology 1 --band 78 -C 3619200000 --ssb 516 \
        --rfsim \
        --uicc0.imsi 208930000000004 \
        --rfsimulator.options chanmod \
        --rfsimulator.serveraddr 10.202.1.100
else
    echo "找到 libtelnetsrv.so，启动带telnet功能的UE..."
    
    # 使用telnet参数启动
    cd ~/openairinterface5g/cmake_targets/ran_build/build
    ./nr-uesoftmodem \
        -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf \
        -r 106 --numerology 1 --band 78 -C 3619200000 --ssb 516 \
        --rfsim \
        --uicc0.imsi 208930000000004 \
        --rfsimulator.options chanmod \
        --rfsimulator.serveraddr 10.202.1.100 \
        --telnetsrv \
        --telnetsrv.listenport 9096
fi
