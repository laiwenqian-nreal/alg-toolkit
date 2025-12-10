#!/bin/bash

set -e

# Get PROJECT_DIR and SELF_DIR
SELF_DIR=`pwd`
read -p "Enter Project Root Directory (default: ../..): " GOTO_PROJECT_PATH
if [ -z "${GOTO_PROJECT_PATH}" ]; then
	GOTO_PROJECT_PATH=../..
fi
cd $GOTO_PROJECT_PATH
PROJECT_DIR=`pwd`

RELATIVE_PATH=""
declare -i PROJECT_DIR_STR_LEN=0
# calculate PROJECT_DIR and SELF_DIR relative path
if [[ "$SELF_DIR" =~ "$PROJECT_DIR" ]]; then
	PROJECT_DIR_STR_LEN=${#PROJECT_DIR}
	RELATIVE_PATH=${SELF_DIR:PROJECT_DIR_STR_LEN + 1}
else
	echo "Fatal Error"
	exit -2
fi

# Get PROJECT_TARGET
echo "ROOT Directory: $PROJECT_DIR"
echo "$(basename $0) Directory: $SELF_DIR"
echo "Relative Path: $RELATIVE_PATH"
echo ""
read -p "Input project main target name(one english word with lower case): " PROJECT_TARGET_INPUT
PROJECT_TARGET_INPUT=${PROJECT_TARGET_INPUT// /-}
if [ -z "${PROJECT_TARGET_INPUT}" ]; then
	echo "Main Target Name is empty!!! nothing generated!"
	exit -1
fi
PROJECT_TARGET=`echo "$PROJECT_TARGET_INPUT" | tr '[:upper:]' '[:lower:]'`
PROJECT_TARGET_UPPER_CASE=`echo "$PROJECT_TARGET_INPUT" | tr '[:lower:]' '[:upper:]'`
PROJECT_TARGET_CAMEL="${PROJECT_TARGET_UPPER_CASE:0:1}${PROJECT_TARGET:1}"
echo "Source Directory ($PROJECT_DIR/$PROJECT_TARGET): "

# Generate Project CMakeLists.txt
PROJECT_CMAKE="$PROJECT_DIR/CMakeLists.txt"
if [ ! -e $PROJECT_CMAKE ]; then
cat > $PROJECT_CMAKE << EOF
cmake_minimum_required( VERSION 3.25.3 )

project(${PROJECT_TARGET}
        VERSION  1.0.0
        DESCRIPTION "${PROJECT_TARGET_CAMEL} project for nrsdk!"
        LANGUAGES CXX C
        )

# Define relative dir for \${CMAKE_SOURCE_DIR}
set(PROJECT_SRC_DIR "${PROJECT_TARGET}")

# find_package(framework CONFIG REQUIRED)
######################################################################################
##                              Compiler											##
######################################################################################
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} xxx")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} xxx")

######################################################################################
##                              Config Project                                      ##
######################################################################################
set(EXTERNAL_PROJECT_DIR \${PROJECT_SOURCE_DIR}/$RELATIVE_PATH)
list( APPEND CMAKE_MODULE_PATH "\${EXTERNAL_PROJECT_DIR}/cmake" )

# After set environment variable, we includ project cmake.
include(Project)

# Set git hook
execute_process(COMMAND git config core.hooksPath $RELATIVE_PATH/githook)
EOF
fi

# Generate compile.sh
COMPILE_SH="$PROJECT_DIR/compile.sh"
if [ ! -e $COMPILE_SH ]; then
ln -s $RELATIVE_PATH/compile.sh $COMPILE_SH
fi

# Generate docker_compile.sh
DOCKER_COMPILE_SH="$PROJECT_DIR/docker_compile.sh"
if [ ! -e $DOCKER_COMPILE_SH ]; then
ln -s $RELATIVE_PATH/docker_compile.sh $DOCKER_COMPILE_SH
fi

# Generate obfuscate.sh
OBFUSCATE_SH="$PROJECT_DIR/obfuscate.sh"
if [ ! -e $OBFUSCATE_SH ]; then
cat > $OBFUSCATE_SH << EOF
#!/bin/bash
set -e
BIN=\${OBFUSCATE_BIN}
find ${PROJECT_TARGET} -name "*.cc" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp"|xargs \$BIN > obfuscate.log
EOF
fi
# Generate .gitignore
GIT_IGNORE="$PROJECT_DIR/.gitignore"
if [ ! -e $GIT_IGNORE ]; then
cat > $GIT_IGNORE << EOF
# Windows:
Thumbs.db
ehthumbs.db
Desktop.ini

# Visual Studio files
.vs
*.sdf
*.opensdf
*.VC.opendb
*.VC.db
*.suo
*.user
_ReSharper.Caches/
Win32-Debug/
Win32-Release/
x64/
x64-Debug/
x64-Release/
Testing/

# MacOS:
.DS_Store

# Android
.gradle
local.properties

# Project
build*/
run.sh
obfuscate.log
install/
tmp

# Misc
.idea/
.vscode
.ccls-cache/
compile_commands.json
.project_alt.json
CMakeUserPresets.json

# Custom

EOF
fi

# Generate conanfile.py
CONANFILE_PY="$PROJECT_DIR/conanfile.py"
if [ ! -e $CONANFILE_PY ]; then
cat > $CONANFILE_PY << EOF
from conan import ConanFile

class Project(ConanFile):

    # all project are the same:
    python_requires = "project_base/1.0"
    python_requires_extend = "project_base.ProjectBase"

    def init(self):
        base = self.python_requires["project_base"].module.ProjectBase
        self.settings = base.settings
        self.options.update(base.options, base.default_options)
        self.revision_mode = base.revision_mode

    # difference between project:
    # def requirements(self):

    # def package_info(self):
EOF
fi

# Generate conan_profile
CONAN="$PROJECT_DIR/conan_profiles"
if [ ! -e $CONAN ]; then
ln -s $RELATIVE_PATH/conan/profiles $CONAN
fi

# Generate source CMakeLists.txt
mkdir -p $PROJECT_DIR/$PROJECT_TARGET
SOURCE_CMAKE="$PROJECT_DIR/$PROJECT_TARGET/CMakeLists.txt"
if [ ! -e $SOURCE_CMAKE ]; then
cat > $SOURCE_CMAKE << EOF
set(EXPORT_BASENAME $PROJECT_TARGET_UPPER_CASE)
set(PLATFORM_CONFIG_BASENAME $PROJECT_TARGET_UPPER_CASE)
set(SYSTEM_CONFIG_BASENAME $PROJECT_TARGET_UPPER_CASE)
set(VERSION_BASENAME $PROJECT_TARGET_UPPER_CASE)

set(CMAKE_CXX_FLAGS "\${CMAKE_CXX_FLAGS} -fno-rtti")

ADD_PLUGIN_LIBRARIES( $PROJECT_TARGET shared_library static_library helloworld)

add_subdirectory(../${RELATIVE_PATH}/cmake/env env)
add_subdirectory(library)
add_subdirectory(helloworld)
EOF
fi

# Generate source library/library.cc
LIBRARY_CC="$PROJECT_DIR/$PROJECT_TARGET/library/library.cc"
if [ ! -e $LIBRARY_CC ]; then
mkdir -p $PROJECT_DIR/$PROJECT_TARGET/library
cat > $LIBRARY_CC << EOF
// TODO: Use correct inlucde file.
#include <$PROJECT_TARGET/env/export.h>
// #include <$PROJECT_TARGET/interface/public/nr_plugin_interface.h>

#ifdef ${PROJECT_TARGET_UPPER_CASE}_SHARED_LIBS
${PROJECT_TARGET_UPPER_CASE}_EXTERN_C_BEGIN
/*
void NRPluginLoad_${PROJECT_TARGET_CAMEL}(NRInterfaces* interfaces);
void NRPluginUnload_${PROJECT_TARGET_CAMEL}();

void NR_INTERFACE_EXPORT NR_INTERFACE_API
NRPluginLoad(NRInterfaces* interfaces) {
	NRPluginLoad_${PROJECT_TARGET_CAMEL}(interfaces);
}
void NR_INTERFACE_EXPORT NR_INTERFACE_API
NRPluginUnload() {
	NRPluginUnload_${PROJECT_TARGET_CAMEL}();
}
*/
${PROJECT_TARGET_UPPER_CASE}_EXTERN_C_END
#endif
EOF
fi

# Generate library/CMakeLists.txt
LIBRARY_CMAKE="$PROJECT_DIR/$PROJECT_TARGET/library/CMakeLists.txt"
if [ ! -e $LIBRARY_CMAKE ]; then
mkdir -p $PROJECT_DIR/$PROJECT_TARGET/library
cat > $LIBRARY_CMAKE << EOF
PROJECT_INIT_TARGET_VARIABLE(shared_library)

add_library( \${TARGET_NAME} OBJECT )

target_sources( \${TARGET_NAME}
	PRIVATE
	"$<BUILD_INTERFACE:\${TARGET_HEADER_FILES}>"
	"$<BUILD_INTERFACE:\${TARGET_SOURCE_FILES}>"
	)

target_include_directories( \${TARGET_NAME}
	PUBLIC
	"$<BUILD_INTERFACE:\${TARGET_INCLUDE_PATH}>"
	"$<BUILD_INTERFACE:\${PROJECT_BINARY_DIR}>"
	"$<BUILD_INTERFACE:\${PROJECT_SOURCE_DIR}>"
	)
target_link_libraries(\${TARGET_NAME}
	PUBLIC
	)
target_compile_definitions(\${TARGET_NAME} PUBLIC \${EXPORT_BASENAME}_SHARED_LIBS)
PROJECT_INSTALL(\${TARGET_NAME} \${TARGETS_EXPORT_NAME})

PROJECT_INIT_TARGET_VARIABLE(static_library)

add_library( \${TARGET_NAME} OBJECT )

target_sources( \${TARGET_NAME}
	PRIVATE
	"$<BUILD_INTERFACE:\${TARGET_HEADER_FILES}>"
	"$<BUILD_INTERFACE:\${TARGET_SOURCE_FILES}>"
	)

target_include_directories( \${TARGET_NAME}
	PUBLIC
	"$<BUILD_INTERFACE:\${TARGET_INCLUDE_PATH}>"
	"$<BUILD_INTERFACE:\${PROJECT_BINARY_DIR}>"
	"$<BUILD_INTERFACE:\${PROJECT_SOURCE_DIR}>"
	)
target_link_libraries(\${TARGET_NAME}
	PUBLIC
	)
PROJECT_INSTALL(\${TARGET_NAME} \${TARGETS_EXPORT_NAME})

######################################################################################
##  							INSTALL  					##
######################################################################################
install(FILES \${TARGET_HEADER_FILES}
	DESTINATION "\${CMAKE_INSTALL_INCLUDEDIR}/\${PROJECT_NAME}/library")

EOF
fi

# Generate source helloworld/helloworld.cc
HELLO_CC="$PROJECT_DIR/$PROJECT_TARGET/helloworld/helloworld.cc"
if [ ! -e $HELLO_CC ]; then
mkdir -p $PROJECT_DIR/$PROJECT_TARGET/helloworld
cat > $HELLO_CC << EOF
#include <$PROJECT_TARGET/env/export.h>

#include <iostream>

void hello() {
  std::cout << "Hello, world!" << std::endl;
}
EOF
fi

# Generate helloworld/CMakeLists.txt
HELLO_CMAKE="$PROJECT_DIR/$PROJECT_TARGET/helloworld/CMakeLists.txt"
if [ ! -e $HELLO_CMAKE ]; then
mkdir -p $PROJECT_DIR/$PROJECT_TARGET/helloworld
cat > $HELLO_CMAKE << EOF
PROJECT_INIT_TARGET_VARIABLE(helloworld)

add_library( \${TARGET_NAME} OBJECT )

target_sources( \${TARGET_NAME}
	PRIVATE
	"$<BUILD_INTERFACE:\${TARGET_HEADER_FILES}>"
	"$<BUILD_INTERFACE:\${TARGET_SOURCE_FILES}>"
	)

target_include_directories( \${TARGET_NAME}
	PUBLIC
	"$<BUILD_INTERFACE:\${TARGET_INCLUDE_PATH}>"
	"$<BUILD_INTERFACE:\${PROJECT_BINARY_DIR}>"
	"$<BUILD_INTERFACE:\${PROJECT_SOURCE_DIR}>"
	)
target_link_libraries(\${TARGET_NAME}
	PUBLIC
	)

######################################################################################
##  							INSTALL  					##
######################################################################################
install(FILES \${TARGET_HEADER_FILES}
	DESTINATION "\${CMAKE_INSTALL_INCLUDEDIR}/\${PROJECT_NAME}/\${TARGET_NAME}")

PROJECT_INSTALL(\${TARGET_NAME} \${TARGETS_EXPORT_NAME})
EOF
fi

# Generate tests CMakeLists.txt
mkdir -p $PROJECT_DIR/tests
TEST_CMAKE="$PROJECT_DIR/tests/CMakeLists.txt"
if [ ! -e $TEST_CMAKE ]; then
cat > $TEST_CMAKE << EOF
add_library( test_base INTERFACE )
target_include_directories( test_base
    INTERFACE
    ${EXTERNAL_INCLUDE_DIR}
    )
target_link_libraries( test_base
    INTERFACE
    doctest::doctest
    ${PROJECT_TARGET}_static
    )

# add tests
#TEST_TARGET(test_xxx test_xxx.cc test_base)
EOF
fi

# Generate apps CMakeLists.txt
mkdir -p $PROJECT_DIR/apps
APP_CMAKE="$PROJECT_DIR/apps/CMakeLists.txt"
if [ ! -e $APP_CMAKE ]; then
cat > $APP_CMAKE << EOF
add_library( app_base INTERFACE )
target_include_directories( app_base
    INTERFACE
    ${EXTERNAL_INCLUDE_DIR}
    )
target_link_libraries( app_base
    INTERFACE
    ${PROJECT_TARGET}_static
    )

# add apps
#APP_TARGET(app_xxx app_xxx.cc app_base)
EOF
fi
