cmake_minimum_required(VERSION 3.10)
project(grpc_receiver CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================
# grpc_client_receiver - gRPC 接收服务端
# ============================================
find_package(protobuf CONFIG)
find_package(gRPC CONFIG)
find_package(framework CONFIG)

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

