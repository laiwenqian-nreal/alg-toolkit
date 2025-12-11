# cmake_minimum_required(VERSION 3.10)
# project(DatadumpTests)

# # 设置C++标准
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# find_package(framework REQUIRED)
# find_package(protobuf CONFIG)
# find_package(gRPC CONFIG)

# # 简单测试（使用你的代码但带stub）
# add_executable(real_tcp_test 
#     real_tcp_test.cpp
# )

# target_include_directories(real_tcp_test PRIVATE 
#     ${CMAKE_CURRENT_SOURCE_DIR}/..
# )

# target_link_libraries(real_tcp_test
#     framework::framework
#     ${CMAKE_BINARY_DIR}/toolkit/datadump/libdatadump.a
# )

# add_executable(real_file_test 
#     real_file_test.cpp
# )

# target_include_directories(real_file_test PRIVATE 
#     ${CMAKE_CURRENT_SOURCE_DIR}/..
# )

# target_link_libraries(real_file_test
#     framework::framework
#     ${CMAKE_BINARY_DIR}/toolkit/datadump/libdatadump.a
# )

# # ============================================
# # send_grpc_test - 动态库加载测试
# # ============================================
# add_executable(send_grpc_test
#     send_grpc_test.cpp
# )

# target_link_libraries(send_grpc_test
#     PRIVATE
#     framework::framework
# )

# set_target_properties(send_grpc_test PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
# )

# # ============================================
# # real_grpc_test - 实际的 gRPC 测试
# # ============================================
# if(protobuf_FOUND AND gRPC_FOUND)
#     add_executable(real_grpc_test
#         real_grpc_test.cpp
#     )

#     # 添加必要的头文件路径
#     target_include_directories(real_grpc_test
#         PRIVATE
#             ${CMAKE_SOURCE_DIR}
#             ${CMAKE_BINARY_DIR}/toolkit/datadump/generated
#     )

#     target_link_libraries(real_grpc_test
#         PRIVATE
#             datadump
#             gRPC::grpc++
#             protobuf::libprotobuf
#             framework::framework
#     )
    
#     set_target_properties(real_grpc_test PROPERTIES
#         RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
#     )
# else()
#     message(WARNING "gRPC or protobuf not found. real_grpc_test will not be built.")
# endif()

# # 构建所有测试的目标
# add_custom_target(all_tests
#     DEPENDS real_tcp_test real_file_test send_grpc_test
#     COMMENT "Building all simple tests"
# )








cmake_minimum_required(VERSION 3.10)
project(DatadumpTests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(framework REQUIRED)
find_package(protobuf CONFIG)
find_package(gRPC CONFIG)


# 1. 修复 real_tcp_test
add_executable(real_tcp_test 
    real_tcp_test.cpp
)
# 新增：依赖 datadump 目标（确保先编译datadump）
add_dependencies(real_tcp_test datadump)
# 新增：添加datadump的头文件路径
target_include_directories(real_tcp_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/toolkit/datadump  # 显式添加datadump的头文件路径
)
target_link_libraries(real_tcp_test
    framework::framework
    datadump  # 直接用datadump目标，CMake会自动处理路径
)


# 2. 修复 real_file_test
add_executable(real_file_test 
    real_file_test.cpp
)
# 新增：依赖 datadump 目标
add_dependencies(real_file_test datadump)
# 新增：添加datadump的头文件路径
target_include_directories(real_file_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/toolkit/datadump
)
# 修复：链接datadump库
target_link_libraries(real_file_test
    framework::framework
    datadump
)


# 3. send_grpc_test 保持不变
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


# 4. real_grpc_test 保持不变（已正确配置）
if(protobuf_FOUND AND gRPC_FOUND)
    add_executable(real_grpc_test
        real_grpc_test.cpp
    )
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

# 将测试注册到CTest中
# add_test(NAME real_tcp_test COMMAND real_tcp_test)
# add_test(NAME real_file_test COMMAND real_file_test)
add_test(NAME send_grpc_test COMMAND send_grpc_test)

# 只在protobuf和gRPC都找到时注册real_grpc_test
if(protobuf_FOUND AND gRPC_FOUND)
   # add_test(NAME real_grpc_test COMMAND real_grpc_test)
endif()