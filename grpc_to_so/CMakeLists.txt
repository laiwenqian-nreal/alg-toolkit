cmake_minimum_required(VERSION 3.10)
project(a_test_for_so LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- 配置：让路径可在 cmake 命令行传入（不想传就直接写死默认值）----
set(DATADUMP_SRC_DIR   "/absolute/path/to/alg-toolkit/toolkit/datadump"            CACHE PATH "datadump source dir")
set(DATADUMP_BUILD_DIR "/absolute/path/to/alg-toolkit/build_native/Release/toolkit/datadump" CACHE PATH "datadump build dir")
set(DATADUMP_LIB       "${DATADUMP_BUILD_DIR}/libdatadump.a")
# 基本存在性检查（防止写错路径）
if(NOT EXISTS "${DATADUMP_SRC_DIR}")
  message(FATAL_ERROR "DATADUMP_SRC_DIR not found: ${DATADUMP_SRC_DIR}")
endif()
if(NOT EXISTS "${DATADUMP_LIB}")
  message(FATAL_ERROR "DATADUMP_LIB not found: ${DATADUMP_LIB}")
endif()

# ---- 关键：找到 protobuf / gRPC（用 Conan 的 CMakeDeps：CONFIG 模式）----
find_package(protobuf REQUIRED CONFIG)   # 提供 protobuf::libprotobuf 以及包含路径
find_package(gRPC     REQUIRED CONFIG)   # 提供 gRPC::grpc++ 等

add_library(send_grpc SHARED
    send_grpc.cpp
)

# 头文件搜索路径：
# 1) datadump 源码目录（比如包含 send_nviz_data_by_grpc.h）
# 2) datadump 的生成目录（protobuf/grpc 生成的 alg.pb.h / alg.grpc.pb.h）
target_include_directories(send_grpc
    PUBLIC
        "${DATADUMP_SRC_DIR}"
        "${DATADUMP_BUILD_DIR}/generated"
)

# 链接 datadump 静态库（用绝对路径）
target_link_libraries(send_grpc
    PUBLIC
        "${DATADUMP_LIB}"
        protobuf::libprotobuf
        gRPC::grpc++
)

# 把 .so 输出到当前 build 的 lib/ 目录
set_target_properties(send_grpc PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# add_executable(send_grpc_test
#     send_grpc_test.cpp
# )

# target_link_libraries(send_grpc_test PRIVATE send_grpc)
# add_dependencies(send_grpc_test send_grpc)