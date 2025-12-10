set(CLANG_TIDY_FILE_ROOT ${PROJECT_SOURCE_DIR}/${PROJECT_SRC_DIR})
file(GLOB_RECURSE ALL_SOURCE_FILES "${CLANG_TIDY_FILE_ROOT}/**/*.cc" "${CLANG_TIDY_FILE_ROOT}/**/*.c" )
file(GLOB_RECURSE ALL_HEADER_FILES "${CLANG_TIDY_FILE_ROOT}/**/*.h")

# ------------------------------------------------------------------------------
# Clang Format
# ------------------------------------------------------------------------------

MACRO(ENABLE_CLANG_FORMAT)
	find_program(FORMAT_PATH clang-format)
	if(FORMAT_PATH)
		message(STATUS "clang-format - code formating             YES ")
		add_custom_target(format
			COMMAND ${FORMAT_PATH} -i ${ALL_SOURCE_FILES} ${ALL_HEADER_FILES} )
	else()
		message(STATUS "clang-format - code formating             NO ")
	endif()
ENDMACRO()
# ------------------------------------------------------------------------------
# Clang Tidy
# ------------------------------------------------------------------------------
MACRO(ENABLE_CLANG_TIDY)

	find_program(CLANG_TIDY_BIN clang-tidy)

    if(CLANG_TIDY_BIN STREQUAL "CLANG_TIDY_BIN-NOTFOUND")
        message(FATAL_ERROR "unable to locate clang-tidy")
    endif()

    list(APPEND RUN_CLANG_TIDY_BIN_ARGS
		-header-filter=${CLANG_TIDY_FILE_ROOT}
        -checks=clan*,cert*,misc*,perf*,cppc*,read*,mode*,-cert-err58-cpp,-misc-noexcept-move-constructor
		-p=./ # build database path
    )

	add_custom_target(
		tidy
		COMMAND ${CLANG_TIDY_BIN} ${RUN_CLANG_TIDY_BIN_ARGS} ${ALL_SOURCE_FILES}
		COMMENT "running clang tidy"
		)

ENDMACRO()

# ------------------------------------------------------------------------------
# Google Sanitizers
# ------------------------------------------------------------------------------

MACRO(ENABLE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=gold")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
ENDMACRO()

MACRO(ENABLE_TSAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=gold")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
ENDMACRO()

# ------------------------------------------------------------------------------
# Valgrind
# ------------------------------------------------------------------------------

MACRO(ENABLE_MEMORY_CHECK)

	find_program(MEMORYCHECK_COMMAND valgrind)

	message("MEMORYCHECK_COMMAND: ${MEMORYCHECK_COMMAND}")

	if(MEMORYCHECK_COMMAND STREQUAL "MEMORYCHECK_COMMAND-NOTFOUND")
		message(FATAL_ERROR "unable to locate valgrind")
	endif()

	set(MEMORYCHECK_COMMAND_OPTIONS "${MEMORYCHECK_COMMAND_OPTIONS} --leak-check=full")
	set(MEMORYCHECK_COMMAND_OPTIONS "${MEMORYCHECK_COMMAND_OPTIONS} --track-fds=yes")
	set(MEMORYCHECK_COMMAND_OPTIONS "${MEMORYCHECK_COMMAND_OPTIONS} --trace-children=yes")
	set(MEMORYCHECK_COMMAND_OPTIONS "${MEMORYCHECK_COMMAND_OPTIONS} --error-exitcode=1")
ENDMACRO()


