#!/usr/bin/env bash

# 如果有任何命令出错，脚本直接退出（推荐）
set -e

cd ~/openairinterface5g
source oaienv
cd cmake_targets

rm -rf ran_build/build

# 直接重新编译
./build_oai --gNB --nrUE -w SIMU

