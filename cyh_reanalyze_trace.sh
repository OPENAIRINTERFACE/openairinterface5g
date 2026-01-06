#!/usr/bin/env bash
# 重新分析已有的 trace 数据（修复过滤器问题）

set -e

if [ -z "$1" ]; then
  # 如果没有指定目录，使用最新的
  TRACE_DIR=$(ls -dt /tmp/oai_gnb_trace_* 2>/dev/null | head -1)
  if [ -z "$TRACE_DIR" ]; then
    echo "❌ 没有找到 trace 数据"
    echo "用法: $0 [trace_directory]"
    exit 1
  fi
else
  TRACE_DIR=$1
fi

if [ ! -d "$TRACE_DIR" ]; then
  echo "❌ 目录不存在: $TRACE_DIR"
  exit 1
fi

echo "================================================"
echo "重新分析 Trace 数据"
echo "================================================"
echo "📍 Trace 目录: $TRACE_DIR"
echo ""

echo "📊 生成调用树（前 2000 行，过滤 C++ 标准库）..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 20 -t 1us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | head -2000 > "$TRACE_DIR/call_tree_full.txt"
echo "✅ 已保存到: $TRACE_DIR/call_tree_full.txt"

echo ""
echo "📈 生成函数统计（前 200 个函数）..."
uftrace report -d "$TRACE_DIR" --no-libcall -s total -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | head -200 > "$TRACE_DIR/function_stats_full.txt"
echo "✅ 已保存到: $TRACE_DIR/function_stats_full.txt"

echo ""
echo "🔍 提取 RRC 相关函数（所有深度）..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 30 -t 0us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | grep -iE 'rrc' | head -1000 > "$TRACE_DIR/rrc_calls_full.txt" || true
echo "✅ 已保存到: $TRACE_DIR/rrc_calls_full.txt ($(wc -l < $TRACE_DIR/rrc_calls_full.txt) 行)"

echo ""
echo "🔍 提取 NGAP 相关函数（所有深度）..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 30 -t 0us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | grep -iE 'ngap' | head -1000 > "$TRACE_DIR/ngap_calls_full.txt" || true
echo "✅ 已保存到: $TRACE_DIR/ngap_calls_full.txt ($(wc -l < $TRACE_DIR/ngap_calls_full.txt) 行)"

echo ""
echo "🔍 提取 NAS 相关函数（所有深度）..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 30 -t 0us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | grep -iE 'nas' | head -1000 > "$TRACE_DIR/nas_calls_full.txt" || true
echo "✅ 已保存到: $TRACE_DIR/nas_calls_full.txt ($(wc -l < $TRACE_DIR/nas_calls_full.txt) 行)"

echo ""
echo "🔍 提取 PDCP 相关函数（所有深度）..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 30 -t 0us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | grep -iE 'pdcp' | head -1000 > "$TRACE_DIR/pdcp_calls_full.txt" || true
echo "✅ 已保存到: $TRACE_DIR/pdcp_calls_full.txt ($(wc -l < $TRACE_DIR/pdcp_calls_full.txt) 行)"

echo ""
echo "🔍 提取 MAC 相关函数（所有深度）..."
uftrace replay -d "$TRACE_DIR" --no-libcall -D 30 -t 0us -N 'std::*' -N '__cxa*' -N '_IO*' 2>/dev/null | grep -iE '\bmac\b|nr_mac|NR_MAC' | head -1000 > "$TRACE_DIR/mac_calls_full.txt" || true
echo "✅ 已保存到: $TRACE_DIR/mac_calls_full.txt ($(wc -l < $TRACE_DIR/mac_calls_full.txt) 行)"

echo ""
echo "================================================"
echo "✅ 分析完成！"
echo "================================================"
echo ""
echo "📁 查看结果："
echo "  完整调用树:  less $TRACE_DIR/call_tree_full.txt"
echo "  函数统计:    less $TRACE_DIR/function_stats_full.txt"
echo "  RRC 调用:    less $TRACE_DIR/rrc_calls_full.txt"
echo "  NGAP 调用:   less $TRACE_DIR/ngap_calls_full.txt"
echo "  NAS 调用:    less $TRACE_DIR/nas_calls_full.txt"
echo "  PDCP 调用:   less $TRACE_DIR/pdcp_calls_full.txt"
echo "  MAC 调用:    less $TRACE_DIR/mac_calls_full.txt"
echo ""
