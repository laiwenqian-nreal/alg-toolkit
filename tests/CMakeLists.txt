cmake_minimum_required(VERSION 3.10)
project(DatadumpTests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(framework REQUIRED)
find_package(protobuf CONFIG)
find_package(gRPC CONFIG)

# 简单测试（使用你的代码但带stub）
add_executable(real_tcp_test 
    real_tcp_test.cpp
)

target_include_directories(real_tcp_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(real_tcp_test
    framework::framework
    ${CMAKE_BINARY_DIR}/toolkit/datadump/libdatadump.a
)

add_executable(real_file_test 
    real_file_test.cpp
)

target_include_directories(real_file_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(real_file_test
    framework::framework
    ${CMAKE_BINARY_DIR}/toolkit/datadump/libdatadump.a
)

# ============================================
# send_grpc_test - 动态库加载测试
# ============================================
add_executable(send_grpc_test
    send_grpc_test.cpp
)

target_link_libraries(send_grpc_test
    PRIVATE
    framework::framework
)

set_target_properties(send_grpc_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ============================================
# real_grpc_test - 实际的 gRPC 测试
# ============================================
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

# 构建所有测试的目标
add_custom_target(all_tests
    DEPENDS real_tcp_test real_file_test send_grpc_test
    COMMENT "Building all simple tests"
)

