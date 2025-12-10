# conan require in conanfile.py: def build_requirements(self): self.tool_requires("protobuf/x.x.x") self.tool_requires(flatbuffers/x.x.x)

MACRO(FRAMEWORK_INIT_PROTO_VARIABLE_INTERNAL _SUPPORT_TYPE _CUSTOM_TARGET_NAME)

set(XML_FILES ${ARGN})
message("_SUPPORT_TYPE:${_SUPPORT_TYPE}")
message("_CUSTOM_TARGET_NAME:${_CUSTOM_TARGET_NAME}")
message("XML_FILES:${XML_FILES}")

find_package (Python3 COMPONENTS Interpreter)
message("Proto Python3_EXECUTABLE:${Python3_EXECUTABLE}")

if (PROTO_GENERATOR_EXEC)
	message("PROTO_GENERATOR_EXEC:${PROTO_GENERATOR_EXEC}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(PROTO_GENERATOR_EXEC ${framework_BIN_DIRS_RELEASE}/generator/main.py)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(PROTO_GENERATOR_EXEC ${framework_BIN_DIRS_DEBUG}/generator/main.py)
else()
	message(FATAL_ERROR "Could not find generator")
endif()

set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto_gen)

if ("" STREQUAL "${_CUSTOM_TARGET_NAME}")
	execute_process(
			COMMAND ${Python3_EXECUTABLE} -c "import os; os.makedirs('${PROTO_GEN_DIR}', exist_ok=True)"
			COMMAND ${Python3_EXECUTABLE} ${PROTO_GENERATOR_EXEC} ${CMAKE_CURRENT_BINARY_DIR} ${XML_FILES}
			RESULTS_VARIABLE ret
	)

	# 检查每个命令的返回码
	foreach(return_code IN LISTS ret)
		if(NOT return_code EQUAL 0)
			message(FATAL_ERROR "XML generator execute_process error:" ${ret})
		endif()
	endforeach()
else ()
	set(PROTO_GEN_STAMP ${PROTO_GEN_DIR}/${_CUSTOM_TARGET_NAME}.stamp)
	if(NOT EXISTS ${GENERATED_STAMP})
		message("Generating protocol sources from XML...")
		execute_process(
				COMMAND ${Python3_EXECUTABLE} -c "import os; os.makedirs('${PROTO_GEN_DIR}', exist_ok=True)"
				COMMAND ${Python3_EXECUTABLE} ${PROTO_GENERATOR_EXEC} ${CMAKE_CURRENT_BINARY_DIR} ${XML_FILES}
				RESULTS_VARIABLE ret
		)

		# 检查每个命令的返回码
		foreach(return_code IN LISTS ret)
			if(NOT return_code EQUAL 0)
				message(FATAL_ERROR "XML generator execute_process error:" ${ret})
			endif()
		endforeach()
	endif ()

	add_custom_command(
			OUTPUT ${PROTO_GEN_STAMP}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${PROTO_GEN_DIR}
			COMMAND ${Python3_EXECUTABLE} ${PROTO_GENERATOR_EXEC} ${CMAKE_CURRENT_BINARY_DIR} ${XML_FILES}
			COMMAND ${CMAKE_COMMAND} -E touch ${PROTO_GEN_STAMP}
			DEPENDS ${XML_FILES} ${PROTO_GENERATOR_EXEC}
			COMMENT "Generating protocol sources from XML..."
			VERBATIM
	)
	add_custom_target(${_CUSTOM_TARGET_NAME} ALL
			DEPENDS ${PROTO_GEN_STAMP}
	)
endif ()

file( GLOB TARGET_PROTO_INLINE_FILES "${CMAKE_CURRENT_BINARY_DIR}/inline/*.*" )
file( GLOB TARGET_PROTO_HEADER_FILES "${CMAKE_CURRENT_BINARY_DIR}/include/*.*" )
file( GLOB TARGET_PROTO_SOURCE_FILES "${CMAKE_CURRENT_BINARY_DIR}/src/*.*" )
file( GLOB TARGET_PROTOBUF_FILES "${CMAKE_CURRENT_BINARY_DIR}/proto/*.proto" )
file( GLOB TARGET_FLATBUFFERS_FILES "${CMAKE_CURRENT_BINARY_DIR}/proto/*.fbs" )

set_source_files_properties(
		${TARGET_PROTO_INLINE_FILES}
		${TARGET_PROTO_HEADER_FILES}
		${TARGET_PROTO_SOURCE_FILES}
		${TARGET_PROTOBUF_FILES}
		${TARGET_FLATBUFFERS_FILES}
		PROPERTIES GENERATED TRUE
)

message("TARGET_PROTOBUF_FILES:${TARGET_PROTOBUF_FILES}")
message("TARGET_FLATBUFFERS_FILES:${TARGET_FLATBUFFERS_FILES}")

if ( "GRPC" STREQUAL "${_SUPPORT_TYPE}" )
	# in conan grpc in p/lib/cmake/conan_trick
	get_target_property(GRPC_CPP_PLUGIN_EXECUTABLE gRPC::grpc_cpp_plugin IMPORTED_LOCATION)

	if (TARGET_PROTOBUF_FILES)
		set(_outvar)
		# in conan protobuf in p/lib/cmake/protobuf/protobuf-generate.cmake
		protobuf_generate(APPEND_PATH 
			LANGUAGE cpp 
			OUT_VAR _outvar 
			PROTOC_OUT_DIR ${PROTO_GEN_DIR} 
			PROTOS ${TARGET_PROTOBUF_FILES} 
			PROTOC_OPTIONS
			"--grpc_out=${PROTO_GEN_DIR}"
			PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}"
		)
		foreach(_file ${_outvar})
			if(_file MATCHES "cc$")
				list(APPEND TARGET_PROTO_SOURCE_FILES_EXT ${_file})
				string(REGEX REPLACE "\\.pb\\.cc$" ".grpc.pb.cc" _cc_file "${_file}")
				list(APPEND TARGET_PROTO_SOURCE_GRPC_FILES_EXT ${_cc_file})
			else()
				list(APPEND TARGET_PROTO_HEADER_FILES_EXT ${_file})
				string(REGEX REPLACE "\\.pb\\.h$" ".grpc.pb.h" _h_file "${_file}")
				list(APPEND TARGET_PROTO_HEADER_GRPC_FILES_EXT ${_h_file})
			endif()
		endforeach()
	endif()
	set_source_files_properties(${TARGET_PROTO_SOURCE_GRPC_FILES_EXT} PROPERTIES GENERATED TRUE)
	set_source_files_properties(${TARGET_PROTO_HEADER_GRPC_FILES_EXT} PROPERTIES GENERATED TRUE)
else ()
	if (TARGET_PROTOBUF_FILES)
		set(_outvar)
		# in conan protobuf in p/lib/cmake/protobuf/protobuf-generate.cmake
		protobuf_generate(APPEND_PATH 
			LANGUAGE cpp 
			OUT_VAR _outvar 
			PROTOC_OUT_DIR ${PROTO_GEN_DIR} 
			PROTOS ${TARGET_PROTOBUF_FILES} 
		)
		foreach(_file ${_outvar})
			if(_file MATCHES "cc$")
				list(APPEND TARGET_PROTO_SOURCE_FILES_EXT ${_file})
			else()
				list(APPEND TARGET_PROTO_HEADER_FILES_EXT ${_file})
			endif()
		endforeach()
	endif()
endif ()

if (TARGET_FLATBUFFERS_FILES)
# in conan in p/.../BuildFlatBuffers.cmake
flatbuffers_generate_headers(
	TARGET proto_gen_flatbuffers
	INCLUDE_PREFIX .
	SCHEMAS ${TARGET_FLATBUFFERS_FILES}
	# FLAGS --gen-object-api
)
endif()

ENDMACRO()

MACRO(FRAMEWORK_INIT_GRPC_PROTO_VARIABLE ...)
	FRAMEWORK_INIT_PROTO_VARIABLE_INTERNAL("GRPC" "" ${ARGV})
ENDMACRO()
MACRO(FRAMEWORK_INIT_PROTO_VARIABLE ...)
	FRAMEWORK_INIT_PROTO_VARIABLE_INTERNAL("NORMAL" "" ${ARGV})
ENDMACRO()
MACRO(FRAMEWORK_INIT_TARGET_GRPC_PROTO_VARIABLE CUSTOM_TARGET_NAME ...)
	FRAMEWORK_INIT_PROTO_VARIABLE_INTERNAL("GRPC" ${ARGV})
ENDMACRO()
MACRO(FRAMEWORK_INIT_TARGET_PROTO_VARIABLE CUSTOM_TARGET_NAME ...)
	FRAMEWORK_INIT_PROTO_VARIABLE_INTERNAL("NORMAL" ${ARGV})
ENDMACRO()
