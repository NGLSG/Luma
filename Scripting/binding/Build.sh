#!/bin/bash

# 脚本说明:
# 这个脚本会为 Windows-x64, Linux-x64 和 Android-arm64 平台发布 .NET 解决方案.
# 所有生成的文件都会被放置在根目录下的 "Tools" 文件夹中.

# 如果任何命令失败，则立即退出脚本
set -e

# 设置解决方案文件名
SOLUTION_NAME="LumaScripting.sln"

# 清理旧的输出目录 (可选)
if [ -d "Tools" ]; then
    echo "Deleting old Tools directory..."
    rm -rf "Tools"
fi

echo "======================================================="
echo "Publishing for Windows (win-x64)..."
echo "======================================================="
dotnet publish -c Release "$SOLUTION_NAME" -r win-x64 -o ./Tools/Win

echo ""
echo "======================================================="
echo "Publishing for Linux (linux-x64)..."
echo "======================================================="
dotnet publish -c Release "$SOLUTION_NAME" -r linux-x64 -o ./Tools/Linux

echo ""
echo "======================================================="
echo "Publishing for Android (android-arm64)..."
echo "======================================================="
dotnet publish -c Release "$SOLUTION_NAME" -r android-arm64 -o ./Tools/Android

echo ""
echo "======================================================="
echo "All publishing tasks are complete."
echo "Check the \"./Tools\" directory for the output files."
echo "======================================================="