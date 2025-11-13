#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="${1:-Release}"

# 脚本所在目录（grpc_to_so）
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
# 项目根目录 alg-toolkit（grpc_to_so 的上一级）
ROOT_DIR="$(realpath "$SCRIPT_DIR/..")"

# 需要传给 grpc_to_so/CMakeLists 的三个路径
DATADUMP_SRC_DIR="$ROOT_DIR/toolkit/datadump"
DATADUMP_BUILD_DIR="$ROOT_DIR/build_native/$BUILD_TYPE/toolkit/datadump"
PREFIX_PATH="$ROOT_DIR/build_native/$BUILD_TYPE/generators"

# 本工程的 build 目录
BUILD_DIR="$SCRIPT_DIR/build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置
cmake \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DDATADUMP_SRC_DIR="$DATADUMP_SRC_DIR" \
  -DDATADUMP_BUILD_DIR="$DATADUMP_BUILD_DIR" \
  -DCMAKE_PREFIX_PATH="$PREFIX_PATH" \
  ..

# 编译
JOBS="$(command -v nproc >/dev/null 2>&1 && nproc || echo 8)"
cmake --build . -j"$JOBS"
