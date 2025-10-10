cmake_minimum_required(VERSION 3.10)
project(DatadumpTests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(framework REQUIRED)

# 简单测试（使用你的代码但带stub）
add_executable(real_tcp_test 
    real_tcp_test.cpp
)

target_include_directories(real_tcp_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(real_tcp_test
    framework::framework
    ${CMAKE_CURRENT_SOURCE_DIR}/../build_native/Release/toolkit/datadump/libdatadump.a
)

add_executable(real_file_test 
    real_file_test.cpp
)

target_include_directories(real_file_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(real_file_test
    framework::framework
    ${CMAKE_CURRENT_SOURCE_DIR}/../build_native/Release/toolkit/datadump/libdatadump.a
)

# 构建所有测试的目标
add_custom_target(all_tests
    DEPENDS real_tcp_test real_file_test
    COMMENT "Building all simple tests"
)
