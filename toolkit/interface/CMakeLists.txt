cmake_minimum_required(VERSION 3.10)
project(toolkit_interface LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- 使用相对路径自动推导 ----
set(DATADUMP_SRC_DIR   "${CMAKE_SOURCE_DIR}/toolkit/datadump" CACHE PATH "datadump source dir")
set(DATADUMP_BUILD_DIR "${CMAKE_BINARY_DIR}/toolkit/datadump" CACHE PATH "datadump build dir")
set(DATADUMP_LIB       "${DATADUMP_BUILD_DIR}/libdatadump.a")

# 基本存在性检查（防止写错路径）
if(NOT EXISTS "${DATADUMP_SRC_DIR}")
  message(WARNING "DATADUMP_SRC_DIR not found: ${DATADUMP_SRC_DIR}, interface libraries may not build correctly")
endif()

# ---- 关键：找到 protobuf / gRPC（用 Conan 的 CMakeDeps：CONFIG 模式）----
find_package(protobuf CONFIG)
find_package(gRPC CONFIG)

# ============================================
# 动态库 1: send_grpc - 仅包含 gRPC 发送功能
# ============================================
if(protobuf_FOUND AND gRPC_FOUND)
    add_library(send_grpc SHARED
        send_grpc_interface.cpp
    )

    target_include_directories(send_grpc
        PUBLIC
            "${DATADUMP_SRC_DIR}"
            "${DATADUMP_BUILD_DIR}/generated"
    )

    target_link_libraries(send_grpc
        PUBLIC
            "${DATADUMP_LIB}"
            protobuf::libprotobuf
            gRPC::grpc++
    )

    # 添加依赖，确保datadump先编译
    add_dependencies(send_grpc datadump)

    set_target_properties(send_grpc PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
else()
    message(WARNING "send_grpc library will not be built: protobuf=${protobuf_FOUND}, gRPC=${gRPC_FOUND}")
endif()

# ============================================
# 动态库 2: save_file - 仅包含文件保存功能
# ============================================
add_library(save_file SHARED
    save_file_interface.cpp
)

target_include_directories(save_file
    PUBLIC
        "${DATADUMP_SRC_DIR}"
)

target_link_libraries(save_file
    PUBLIC
        "${DATADUMP_LIB}"
)

# 添加依赖，确保datadump先编译
add_dependencies(save_file datadump)

set_target_properties(save_file PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# ============================================
# 动态库 3: datadump_toolkit - 包含文件保存和 gRPC 发送
# ============================================
if(protobuf_FOUND AND gRPC_FOUND)
    add_library(datadump_toolkit SHARED
        save_file_interface.cpp
        send_grpc_interface.cpp
    )

    target_include_directories(datadump_toolkit
        PUBLIC
            "${DATADUMP_SRC_DIR}"
            "${DATADUMP_BUILD_DIR}/generated"
    )

    target_link_libraries(datadump_toolkit
        PUBLIC
            "${DATADUMP_LIB}"
            protobuf::libprotobuf
            gRPC::grpc++
    )

    # 添加依赖，确保datadump先编译
    add_dependencies(datadump_toolkit datadump)

    set_target_properties(datadump_toolkit PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
else()
    message(WARNING "datadump_toolkit library will not be built: protobuf=${protobuf_FOUND}, gRPC=${gRPC_FOUND}")
endif()

# add_executable(send_grpc_test
#     send_grpc_test.cpp
# )

# target_link_libraries(send_grpc_test PRIVATE send_grpc)
# add_dependencies(send_grpc_test send_grpc)