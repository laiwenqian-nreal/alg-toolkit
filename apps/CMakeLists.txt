cmake_minimum_required(VERSION 3.10)
project(grpc_receiver CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# 查找依赖
# ============================================
find_package(protobuf CONFIG)
find_package(gRPC CONFIG)
find_package(framework CONFIG)

# 使用本地编译的 ZeroMQ 库
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")
set(ZEROMQ_DIR "${EXTERNAL_DIR}/lib/zeromq")

find_library(ZMQ_LIBRARIES
    NAMES libzmq.a zmq
    PATHS ${ZEROMQ_DIR}/lib
    NO_DEFAULT_PATH 
    REQUIRED
)

find_path(ZMQ_INCLUDE_DIRS
    NAMES zmq.h
    PATHS ${ZEROMQ_DIR}/include
    REQUIRED
)

if(ZMQ_LIBRARIES AND ZMQ_INCLUDE_DIRS)
    message(STATUS "✓ Found ZeroMQ library: ${ZMQ_LIBRARIES}")
    message(STATUS "✓ ZeroMQ include directory: ${ZMQ_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR 
        "ZeroMQ library or headers not found in ${ZEROMQ_DIR}\n"
        "Please ensure the compiled ZeroMQ library and headers are in:\n"
        "  Library:  ${ZEROMQ_DIR}/lib/libzmq.a\n"
        "  Headers:  ${ZEROMQ_DIR}/include/zmq.h and zmq.hpp\n"
    )
endif()


message(STATUS "ZEROMQ_DIR: ${ZEROMQ_DIR}")
message(STATUS "实际找到的 ZMQ_LIBRARIES: ${ZMQ_LIBRARIES}")

# ============================================
# 查找 cpp-refactor 库和头文件
# ============================================
# 修复：使用 CMAKE_SOURCE_DIR 指向项目根目录
# 这样 EXTERNAL_DIR 会正确指向项目根目录下的 external 目录
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")

# 调试：打印实际的搜索路径，方便排查
message(STATUS "🔍 Searching echo_message_system in: ${EXTERNAL_DIR}/lib")

# 查找 echo_message_system 库
# 移除 NO_DEFAULT_PATH（除非确认绝对不需要系统路径），保留显式 PATHS 即可
find_library(ECHO_MSG_LIB 
    NAMES echo_message_system
    PATHS ${EXTERNAL_DIR}/lib
    REQUIRED
)

if(ECHO_MSG_LIB)
    set(ECHO_MSG_LIBRARIES ${ECHO_MSG_LIB})
    set(ECHO_MSG_INCLUDE_DIRS ${EXTERNAL_DIR}/include)
    message(STATUS "✓ Found echo_message_system library: ${ECHO_MSG_LIBRARIES}")
    message(STATUS "✓ Include directory: ${ECHO_MSG_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR 
        "echo_message_system library not found in ${EXTERNAL_DIR}/lib\n"
        "Please place the cpp-refactor compiled library and headers in:\n"
        "  Library:  ${EXTERNAL_DIR}/lib/libecho_message_system.so*\n"
        "  Headers:  ${EXTERNAL_DIR}/include/\n"
    )
endif()

# ============================================
# grpc_client_receiver - gRPC 接收服务端
# ============================================
if(protobuf_FOUND AND gRPC_FOUND)
    add_executable(grpc_client_receiver
        grpc_client_receiver.cpp
    )

    # 添加必要的头文件路径
    target_include_directories(grpc_client_receiver
        PRIVATE
            ${CMAKE_SOURCE_DIR}
            ${CMAKE_BINARY_DIR}/toolkit/datadump/generated  # For alg.pb.h, alg.grpc.pb.h
    )

    target_link_libraries(grpc_client_receiver
        PRIVATE
            datadump
            gRPC::grpc++
            protobuf::libprotobuf
            framework::framework
    )
    
else()
    message(WARNING "gRPC or protobuf not found. grpc_client_receiver will not be built.")
endif()

# ============================================
# grpc_receiver_send_by_topic - gRPC 接收并通过 Topic 发布
# ============================================
if(protobuf_FOUND AND gRPC_FOUND AND ECHO_MSG_LIB)
    add_executable(grpc_receiver_send_by_topic
        grpc_receiver_send_by_topic.cpp
    )

    # 添加必要的头文件路径
    target_include_directories(grpc_receiver_send_by_topic
        PRIVATE
            ${CMAKE_SOURCE_DIR}
            ${CMAKE_BINARY_DIR}/toolkit/datadump/generated  # For alg.pb.h, alg.grpc.pb.h
            ${ECHO_MSG_INCLUDE_DIRS}
            ${ZMQ_INCLUDE_DIRS}
    )

    target_link_libraries(grpc_receiver_send_by_topic
        PRIVATE
            datadump
            gRPC::grpc++
            protobuf::libprotobuf
            framework::framework
            ${ECHO_MSG_LIBRARIES}
            pthread
    )

    # 设置 RPATH 确保运行时使用本地 ZeroMQ 库
    # libecho_message_system.so 已经用本地 zeromq 编译，不需要再显式链接 ZeroMQ
    set_target_properties(grpc_receiver_send_by_topic
        PROPERTIES
            BUILD_RPATH "${ZEROMQ_DIR}/lib"
            INSTALL_RPATH "${ZEROMQ_DIR}/lib"
    )

    message(STATUS "✓ grpc_receiver_send_by_topic will be built with:")
    message(STATUS "  echo_message_system: ${ECHO_MSG_LIBRARIES}")
    message(STATUS "  ZMQ: ${ZMQ_LIBRARIES}")
    message(STATUS "  ZMQ RPATH: ${ZEROMQ_DIR}/lib")
    
else()
    message(WARNING "gRPC, protobuf or echo_message_system not found. grpc_receiver_send_by_topic will not be built.")
endif()