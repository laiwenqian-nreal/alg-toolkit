cmake_minimum_required(VERSION 3.10)
project(send_grpc_test_only CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(send_grpc_test
    send_grpc_test.cpp
)

if(NOT DEFINED SEND_GRPC_LIBDIR)
    message(FATAL_ERROR "Please set -DSEND_GRPC_LIBDIR=/absolute/path/to/lib when running cmake")
endif()
set(SEND_GRPC_SO "${SEND_GRPC_LIBDIR}/libsend_grpc.so")
if(NOT EXISTS "${SEND_GRPC_SO}")
    message(FATAL_ERROR "Not found: ${SEND_GRPC_SO}")
endif()

target_link_libraries(send_grpc_test
    PRIVATE
        "${SEND_GRPC_SO}"
)

set_target_properties(send_grpc_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY    "${CMAKE_BINARY_DIR}/bin"
    BUILD_RPATH                 "${SEND_GRPC_LIBDIR}"
)
