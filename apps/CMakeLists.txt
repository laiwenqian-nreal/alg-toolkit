# 应用程序目录

# gRPC 测试程序（合并了 grpc_to_so 和 grpc_send_test）
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/grpc/CMakeLists.txt)
    add_subdirectory(grpc)
endif()

# 其他测试程序
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()
