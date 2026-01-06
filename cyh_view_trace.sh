#!/usr/bin/env bash
# 快速查看最近一次 trace 的调用树

set -e

# 查找最新的 trace 目录
LATEST_GNB=$(ls -dt /tmp/oai_gnb_trace_* 2>/dev/null | head -1)
LATEST_UE=$(ls -dt /tmp/oai_ue_trace_* 2>/dev/null | head -1)

echo "================================================"
echo "查看 uftrace 调用树"
echo "================================================"
echo ""
echo "请选择要查看的 trace："
echo "  1) gNB trace (最新: ${LATEST_GNB:-无})"
echo "  2) UE trace  (最新: ${LATEST_UE:-无})"
echo "  3) 指定目录"
echo ""
read -p "选择 [1-3]: " choice

case $choice in
  1)
    if [ -z "$LATEST_GNB" ]; then
      echo "❌ 没有找到 gNB trace 数据"
      exit 1
    fi
    TRACE_DIR="$LATEST_GNB"
    ;;
  2)
    if [ -z "$LATEST_UE" ]; then
      echo "❌ 没有找到 UE trace 数据"
      exit 1
    fi
    TRACE_DIR="$LATEST_UE"
    ;;
  3)
    read -p "输入 trace 目录路径: " TRACE_DIR
    ;;
  *)
    echo "❌ 无效选择"
    exit 1
    ;;
esac

if [ ! -d "$TRACE_DIR" ]; then
  echo "❌ 目录不存在: $TRACE_DIR"
  exit 1
fi

echo ""
echo "================================================"
echo "使用的 trace 目录: $TRACE_DIR"
echo "================================================"
echo ""
echo "请选择查看方式："
echo "  1) 完整调用树 (所有函数，深度 15)"
echo "  2) RRC 相关函数 (过滤 nr_rrc_*)"
echo "  3) NAS 相关函数 (过滤 nas_*)"
echo "  4) NGAP 相关函数 (过滤 ngap_*)"
echo "  5) 自定义函数过滤"
echo "  6) 生成 Chrome Trace JSON"
echo "  7) 生成 GraphViz 调用图"
echo "  8) 函数统计 (按耗时排序)"
echo ""
read -p "选择 [1-8]: " view_choice

case $view_choice in
  1)
    echo "📊 显示完整调用树..."
    uftrace replay -d "$TRACE_DIR" --srcline --no-libcall -D 15 -t 5us | less
    ;;
  2)
    echo "📊 显示 RRC 相关函数..."
    uftrace replay -d "$TRACE_DIR" --srcline --no-libcall -D 20 -t 0us -F 'nr_rrc_.*' | less
    ;;
  3)
    echo "📊 显示 NAS 相关函数..."
    uftrace replay -d "$TRACE_DIR" --srcline --no-libcall -D 20 -t 0us -F 'nas_.*' | less
    ;;
  4)
    echo "📊 显示 NGAP 相关函数..."
    uftrace replay -d "$TRACE_DIR" --srcline --no-libcall -D 20 -t 0us -F 'ngap_.*' | less
    ;;
  5)
    read -p "输入函数名模式 (支持通配符，如 'rrc_*Setup*'): " pattern
    echo "📊 显示匹配 '$pattern' 的函数..."
    uftrace replay -d "$TRACE_DIR" --srcline --no-libcall -D 20 -t 0us -F "$pattern" | less
    ;;
  6)
    OUTPUT="$TRACE_DIR/trace.json"
    echo "🎨 生成 Chrome Trace..."
    uftrace dump -d "$TRACE_DIR" --chrome > "$OUTPUT"
    echo "✅ 已保存到: $OUTPUT"
    echo ""
    echo "💡 使用方法："
    echo "   1. 在 Chrome 浏览器中打开: chrome://tracing"
    echo "   2. 点击 'Load' 按钮"
    echo "   3. 选择文件: $OUTPUT"
    ;;
  7)
    OUTPUT_DOT="$TRACE_DIR/callgraph.dot"
    OUTPUT_PNG="$TRACE_DIR/callgraph.png"
    echo "📈 生成 GraphViz 调用图..."
    uftrace dump -d "$TRACE_DIR" --graphviz > "$OUTPUT_DOT"
    echo "✅ DOT 文件已保存到: $OUTPUT_DOT"
    
    if command -v dot &> /dev/null; then
      echo "📈 生成 PNG 图片..."
      dot -Tpng "$OUTPUT_DOT" -o "$OUTPUT_PNG"
      echo "✅ PNG 图片已保存到: $OUTPUT_PNG"
    else
      echo "⚠️  graphviz 未安装，无法生成 PNG"
      echo "   安装命令: sudo apt install -y graphviz"
      echo "   或者下载 DOT 文件手动转换"
    fi
    ;;
  8)
    echo "📊 函数统计信息（按总耗时排序）..."
    uftrace report -d "$TRACE_DIR" --srcline --no-libcall -s total | less
    ;;
  *)
    echo "❌ 无效选择"
    exit 1
    ;;
esac

echo ""
echo "================================================"
echo "✅ 完成"
echo "================================================"
