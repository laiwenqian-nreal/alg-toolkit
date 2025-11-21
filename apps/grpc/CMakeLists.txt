cmake_minimum_required(VERSION 3.10)
project(grpc_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# send_grpc_test - 需要预编译的 libsend_grpc.so
# ============================================
add_executable(send_grpc_test
    send_grpc_test.cpp
)

# 链接 dl 库用于 dlopen/dlsym
target_link_libraries(send_grpc_test
    PRIVATE
    framework::framework
)

# ============================================
# real_grpc_test - 实际的 gRPC 测试
# ============================================
find_package(protobuf CONFIG)
find_package(gRPC CONFIG)
find_package(framework CONFIG)

if(protobuf_FOUND AND gRPC_FOUND)
    add_executable(real_grpc_test
        real_grpc_test.cpp
    )

    # 添加必要的头文件路径
    target_include_directories(real_grpc_test
        PRIVATE
            ${CMAKE_SOURCE_DIR}
            ${CMAKE_BINARY_DIR}/toolkit/datadump/generated
    )

    target_link_libraries(real_grpc_test
        PRIVATE
            datadump
            gRPC::grpc++
            protobuf::libprotobuf
            framework::framework
    )
    
    set_target_properties(real_grpc_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
else()
    message(WARNING "gRPC or protobuf not found. real_grpc_test will not be built.")
endif()

# ============================================
# 输出目录设置
# ============================================
set_target_properties(send_grpc_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
