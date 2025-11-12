PROJECT_INIT_TARGET_VARIABLE(datadump)

# ---- 引入 Conan 的包配置：protobuf / gRPC（使用 Conan 的 CMakeDeps 产物）----
find_package(protobuf REQUIRED CONFIG)
find_package(gRPC     REQUIRED CONFIG)

# ---- Proto 定义（本目录下）----
set(PROTO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/alg.proto)

# 生成文件输出目录（构建树）
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_DIR})

# 生成产物路径
set(GEN_PB_SRCS   ${GEN_DIR}/alg.pb.cc)
set(GEN_PB_HDRS   ${GEN_DIR}/alg.pb.h)
set(GEN_GRPC_SRCS ${GEN_DIR}/alg.grpc.pb.cc)
set(GEN_GRPC_HDRS ${GEN_DIR}/alg.grpc.pb.h)

# ---- 用 Conan 的 protoc 与 grpc_cpp_plugin 生成 ----
add_custom_command(
    OUTPUT
        ${GEN_PB_SRCS} ${GEN_PB_HDRS}
        ${GEN_GRPC_SRCS} ${GEN_GRPC_HDRS}
    COMMAND protobuf::protoc
            -I ${CMAKE_CURRENT_SOURCE_DIR}
            --cpp_out=${GEN_DIR}
            --grpc_out=${GEN_DIR}
            --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
            ${PROTO_SRC}
    DEPENDS ${PROTO_SRC}
    COMMENT "Generating protobuf & gRPC C++ files from alg.proto (datadump)"
    VERBATIM
)

add_library( ${TARGET_NAME} STATIC)

target_sources( ${TARGET_NAME}
	PRIVATE
	${GEN_PB_SRCS}
	${GEN_GRPC_SRCS}
	"$<BUILD_INTERFACE:${TARGET_SOURCE_FILES}>"
	PUBLIC
	"$<BUILD_INTERFACE:${TARGET_HEADER_FILES}>"
	)

target_include_directories( ${TARGET_NAME}
	PUBLIC
	$<BUILD_INTERFACE:${GEN_DIR}>
	"$<BUILD_INTERFACE:${TARGET_INCLUDE_PATH}>"
	"$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>"
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
	)

# 只对本目标关闭 RTTI，避免把选项传播到 protobuf/gRPC 等第三方
target_compile_options(${TARGET_NAME} PRIVATE -fno-rtti)

target_link_libraries(${TARGET_NAME}
	PUBLIC
    framework::framework
	Eigen3::Eigen
	protobuf::libprotobuf
	gRPC::grpc++
)


######################################################################################
##  							INSTALL  											##
######################################################################################
install(FILES ${TARGET_HEADER_FILES}
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${TARGET_NAME}")

# Install threadsafe_queue headers
install(DIRECTORY threadsafe_queue/
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${TARGET_NAME}/threadsafe_queue"
	FILES_MATCHING PATTERN "*.h")

PROJECT_INSTALL(${TARGET_NAME} ${TARGETS_EXPORT_NAME})

######################################################################################
##  							TEST  											  ##
######################################################################################
if(BUILD_TEST)
	add_subdirectory(tests)
endif()