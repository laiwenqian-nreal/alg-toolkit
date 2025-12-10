######################################################################################
#cmake_minimum_required( VERSION 3.13 )
#project( Example
#        VERSION  1.0.0
#		 DESCRIPTION "Example project for nrsdk!" 
#		 LANGUAGES CXX C
#		 )

# Define relative dir for ${PROJECT_SOURCE_DIR}
# set(PROJECT_SRC_DIR "example")

# find_package(framework CONFIG REQUIRED)
# CMAKE_CXX_FLAGS & CMAKE_C_FLAGS

message("Building with CMake version: ${CMAKE_VERSION}")
message("PROJECT_SRC_DIR: ${PROJECT_SRC_DIR}")
######################################################################################
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
   message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

# Encourage user to specify a build type (e.g. Release, Debug, etc.), otherwise set it to Release.
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to 'Release' as no build type was specified")
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type (Debug/Release)" FORCE)
endif()


######################################################################################
if(NOT DEFINED EXTERNAL_PROJECT_DIR)
set(EXTERNAL_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/project)
endif()

message("EXTERNAL_PROJECT_DIR: ${EXTERNAL_PROJECT_DIR}")

list( APPEND CMAKE_MODULE_PATH "${EXTERNAL_PROJECT_DIR}/cmake/cmake/common" )
list( APPEND CMAKE_MODULE_PATH "${EXTERNAL_PROJECT_DIR}/cmake/cmake" )

include( CMakeFindBinUtils )

include( CMakeDependentOption )

# use cmake_print_properties(...) cmake_print_variables(...)
include( CMakePrintHelpers )

include( ExternalProject )
#
# self definition
#
include( ProtoMacro )

include( CCacheLoad )

include( PlatformChecks )

include( ProjectMacro )

include( GetGitRevisionDescription )

include( TestUtil )

include( Util )

include( StripSymbols )

############################
# install relative include 
############################

# Introduce variables:
# * CMAKE_INSTALL_LIBDIR
# * CMAKE_INSTALL_BINDIR
# * CMAKE_INSTALL_INCLUDEDIR
include( GNUInstallDirs )

# Include module with fuction 'write_basic_package_version_file'
include(CMakePackageConfigHelpers)


######################################################################################
##                              PACKAGES	                                        ##
######################################################################################

set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

find_package (Python3 COMPONENTS Interpreter)
######################################################################################
##  							OPTIONS  											##
######################################################################################

option(BUILD_SHARED_LIBS    "Build shared instead of static libraries."             OFF)
option(BUILD_APPS			"Build apps."                                       	OFF)
option(BUILD_DOCS      		"Build documentation."                                  OFF)
option(BUILD_TESTS      	"Build test."                                  		OFF)
option(ENABLE_COVERAGE 		"Add coverage information."                             OFF)
option(ENABLE_CC_CHECK 		"Add CC Check."                             		OFF)
option(ENABLE_REL_WITH_DEB_INFO		"RelWithDebInfo."                             		ON)

option(INSTALL_EXPORT_CMAKE 	"Export CMake related file while install."              OFF)

set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

# For install and export
set(TARGETS_PACKAGE_NAME "${PROJECT_NAME}")
set(TARGETS_EXPORT_NAME "${TARGETS_PACKAGE_NAME}Targets")
set(TARGETS_EXPORT_NAMESPACE "${PROJECT_NAME}::")

# Be nice and export compile commands by default, this is handy for clang-tidy
# and for other tools.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Convert GNU import libraries (.dll.a) to MS format (.lib).
set(CMAKE_GNUtoMS ON)
# Enable verbose makefiles by default.
set(CMAKE_VERBOSE_MAKEFILE ON)

######################################################################################
##  							GIT 												##
######################################################################################

get_git_head_revision(GIT_REFSPEC GIT_SHA1)
string(SUBSTRING "${GIT_SHA1}" 0 12 GIT_REV)
if(NOT GIT_SHA1)
    set(GIT_REV "0")
endif()

######################################################################################
##  							COMPILE FLAGS										##
######################################################################################

# Use c++17 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable C and C++ compiler extensions.
# C/CXX_EXTENSIONS are ON by default to allow the compilers to use extended
# variants of the C/CXX language.
# However, this could expose cross-platform bugs in user code or in the headers
# of third-party dependencies and thus it is strongly suggested to turn
# extensions off.
set( CMAKE_C_EXTENSIONS OFF )
set( CMAKE_CXX_EXTENSIONS OFF )

if (ENABLE_COVERAGE AND ENABLE_CC_CHECK )
	message(FATAL_ERROR "code coverage and code check both modify compile flags!")
endif()
# Init compile flags
if (ENABLE_COVERAGE AND NOT CMAKE_CONFIGURATION_TYPES)
	if (NOT BUILD_TESTS)
		message(FATAL_ERROR "Tests must be enabled for code coverage!")
	endif ()

	include(CodeCoverage)

	append_coverage_compiler_flags()
	set(COVERAGE_INCLUDES "${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}")

	setup_target_for_coverage(NAME coverage EXECUTABLE ctest DEPENDENCIES coverage)
elseif(NOT ENABLE_CC_CHECK)
	message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

		# delete -g -O2 in ${CMAKE_CXX_FLAGS} which may be added in android.toolchain.cmake

		REMOVE_STRING_FROM_VARIABLE(CMAKE_C_FLAGS "\-g")
		REMOVE_STRING_FROM_VARIABLE(CMAKE_CXX_FLAGS "\-g")

		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -ggdb")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -ggdb")

		REMOVE_STRING_FROM_VARIABLE(CMAKE_C_FLAGS_RELEASE "\-O2 ")
		REMOVE_STRING_FROM_VARIABLE(CMAKE_CXX_FLAGS_RELEASE "\-O2 ")

		REMOVE_STRING_FROM_VARIABLE(CMAKE_C_FLAGS_RELEASE "\-O3 ")
		REMOVE_STRING_FROM_VARIABLE(CMAKE_CXX_FLAGS_RELEASE "\-O3 ")

		REMOVE_STRING_FROM_VARIABLE(CMAKE_C_FLAGS_RELEASE "\-DNDEBUG")
		REMOVE_STRING_FROM_VARIABLE(CMAKE_CXX_FLAGS_RELEASE "\-DNDEBUG")

		if (ENABLE_REL_WITH_DEB_INFO)
			set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")
			set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
		endif()
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

		if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PROJECT_CMAKE_C_FLAGS}")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROJECT_CMAKE_CXX_FLAGS}")
			if (WIN32)
			    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gcodeview -Wno-unused-command-line-argument")
			    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gcodeview -Wno-unused-command-line-argument")
			endif()
		elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PROJECT_CMAKE_C_FLAGS}")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROJECT_CMAKE_CXX_FLAGS}")
		endif()

	elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PROJECT_CMAKE_C_FLAGS}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 ${PROJECT_CMAKE_CXX_FLAGS}")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
	endif ()
endif()

# Always use '-fPIC'/'-fPIE' option.
set( CMAKE_POSITION_INDEPENDENT_CODE ON )
# Distinguish between Release lib and Debug lib
# set(DEBUG_POSTFIX d)
# Symbols will not export. Like "-fvisibility=hidden" 
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
# Symbols will not export. Like "-fvisibility-inlines-hidden=1"
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
# It is always easier to navigate in an IDE when projects are organized in folders.
set(USE_FOLDERS ON)


######################################################################################
##  							INSTALL  											##
######################################################################################

set(CMAKE_INSTALL_LIBDIR "lib")

# Layout. This works for all platforms:
#   * <prefix>/lib*/cmake/<PROJECT-NAME>
#   * <prefix>/lib*/
#   * <prefix>/include/
set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Configuration
set(VERSION_CONFIG "${GENERATED_DIR}/${TARGETS_PACKAGE_NAME}ConfigVersion.cmake")
set(PROJECT_CONFIG "${GENERATED_DIR}/${TARGETS_PACKAGE_NAME}Config.cmake")
set(TARGETS_CONFIG "${TARGETS_PACKAGE_NAME}Config.cmake")

if (INSTALL_EXPORT_CMAKE)
# Configure '<PROJECT-NAME>ConfigVersion.cmake'
# Use:
#   * PROJECT_VERSION
write_basic_package_version_file(
	"${VERSION_CONFIG}" COMPATIBILITY SameMinorVersion
)

# Configure '<PROJECT-NAME>Config.cmake'
# Use variables:
#   * TARGETS_EXPORT_NAME
#   * PROJECT_NAME
configure_package_config_file(
    "${EXTERNAL_PROJECT_DIR}/cmake/cmake/common/Config.cmake.in"
    "${PROJECT_CONFIG}"
    INSTALL_DESTINATION "${CONFIG_INSTALL_DIR}"
)

# Config
#   * <prefix>/lib/cmake/<PROJECT-NAME>/<PROJECT-NAME>Config.cmake
#   * <prefix>/lib/cmake/<PROJECT-NAME>/<PROJECT-NAME>ConfigVersion.cmake
install(
	FILES "${PROJECT_CONFIG}" "${VERSION_CONFIG}"
	DESTINATION "${CONFIG_INSTALL_DIR}"
)

# Config
#   * <prefix>/lib/cmake/xxx/xxxTargets.cmake
install(
	EXPORT "${TARGETS_EXPORT_NAME}"
	NAMESPACE "${TARGETS_EXPORT_NAMESPACE}"
	DESTINATION "${CONFIG_INSTALL_DIR}"
)
endif()

######################################################################################
##  							EXPORT BUILD DIRECTORY 								##
######################################################################################

if (INSTALL_EXPORT_CMAKE)
# CMake find_package appears to ignore the User/System Package Registry when using an Android NDK tool chain
# maybe because of CMAKE_FIND_ROOT_PATH
if(NOT ANDROID)
# export build directory targets file
export(
	EXPORT ${TARGETS_EXPORT_NAME}
	NAMESPACE "${TARGETS_EXPORT_NAMESPACE}"
	FILE ${TARGETS_CONFIG}
)

# register project in CMake user registry
export(PACKAGE ${TARGETS_PACKAGE_NAME})
endif()
endif()
######################################################################################
##  					BUILD TIME												 	##
######################################################################################

#use local build date
if (CMAKE_BUILD_TYPE STREQUAL "Release")
	string(TIMESTAMP PROJECT_BUILD_TIME "%Y%m%d%H%M%S")
else()
	string(TIMESTAMP PROJECT_BUILD_TIME "%Y-%m-%d.%H:%M:%S.debug")
endif()

######################################################################################
##  					LOG CMAKE VARIABLES											##
######################################################################################
# include( LogCMakeVariables )

######################################################################################
##  							CC_CHECK 											##
######################################################################################

# example:
# - make tidy
option(ENABLE_CLANG_TIDY   "Clang tidy check."  OFF)

# example:
# - ctest -VV
option(ENABLE_ASAN         "ADDRESS_SANITIZER." OFF)
option(ENABLE_TSAN         "THREAD_SANITIZER."  OFF)

# example:
# - ctest -T memcheck -VV
option(ENABLE_MEMORY_CHECK "Valgrind check."    OFF)

include(CCCheckMacro)
ENABLE_CLANG_FORMAT()
if(ENABLE_CC_CHECK)
	if(ENABLE_CLANG_TIDY)
		ENABLE_CLANG_TIDY()
	endif()
	if(ENABLE_ASAN)
		ENABLE_ASAN()
	endif()
	if(ENABLE_USAN)
		ENABLE_USAN()
	endif()
	if(ENABLE_TSAN)
		ENABLE_TSAN()
	endif()
	if(ENABLE_MEMORY_CHECK)
		if (NOT BUILD_TESTS)
			message(FATAL_ERROR "Tests must be enabled for code ENABLE_MEMORY_CHECK!")
		endif ()
		ENABLE_MEMORY_CHECK()
	endif()
endif()

######################################################################################
##                              RPATH                                               ##
######################################################################################
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# note: macOS is APPLE and also UNIX !
# multi rpath
if(APPLE)
	set(CMAKE_INSTALL_RPATH "@loader_path;./")
elseif(UNIX)
    set(CMAKE_INSTALL_RPATH "$ORIGIN:./")
endif()

######################################################################################
##  							EXTERNAL  											##
######################################################################################

# External dependencies.
# add_subdirectory(external)

######################################################################################
##  							LIBRARY  											##
######################################################################################

# Create targets for building the (local) libraries.
add_subdirectory( ${PROJECT_SRC_DIR} )

######################################################################################
##  							APPS  											##
######################################################################################

if( BUILD_APPS )
	add_subdirectory( apps )
endif()

######################################################################################
##  							TESTS  												##
######################################################################################
if(BUILD_TESTS)
	# Must be called before adding tests but after calling project(). This automatically calls enable_testing() and configures ctest targets when using Make/Ninja
	include(CTest)
	enable_testing()
    # Let the user add options to the test runner if needed
    set(TEST_RUNNER_PARAMS "--force-colors=true" CACHE STRING "Options to add to our test runners commands")
    # In a real project you most likely want to exclude test folders
    # list(APPEND CUSTOM_COVERAGE_EXCLUDE "/test/")
    add_subdirectory(tests)
    # You can setup some custom variables and add them to the CTestCustom.cmake.in template to have custom ctest settings
    # For example, you can exclude some directories from the coverage reports such as third-parties and tests
	# ctest_read_custom_files() : Read all the CTestCustom.ctest or CTestCustom.cmake files from the given directory. By default, invoking ctest() without a script will read custom files from the binary directory
    configure_file(
		${CMAKE_CURRENT_LIST_DIR}/cmake/common/CTestCustom.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake
		@ONLY
    )
endif()

######################################################################################
##  							DOC 												##
######################################################################################
if (NOT DEFINED USE_DEFAULT_DOC)
	set(USE_DEFAULT_DOC ON)
endif()

if(BUILD_DOCS AND USE_DEFAULT_DOC)
	# config ${EXTERNAL_REQUIRE_DOC_DIR} to indicate extra source code dir.
	set(EXTERNAL_REQUIRE_DOC_DIR "${EXTERNAL_REQUIRE_DOC_DIR}")
	add_subdirectory( ${EXTERNAL_PROJECT_DIR}/cmake/doc doc )
endif()

