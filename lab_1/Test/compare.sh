#!/bin/bash

# 检查是否提供了两个文件参数
if [ "$#" -ne 2 ]; then
  echo "Usage: $0 file1 file2"
  exit 1
fi

file1=$1
file2=$2

# 使用diff命令比较文件，-y参数用于逐行比较
diff_output=$(diff -y --suppress-common-lines "$file1" "$file2")

if [ -z "$diff_output" ]; then
  echo "Success: 两个文件内容完全一致"
else
  # 显示不同的行号
  echo "文件不一致的行号为："
  diff -y --suppress-common-lines -n "$file1" "$file2" | awk '{print $1}'
fi
