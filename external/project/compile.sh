#!/bin/bash

#	Usage:
# 	compile.sh [-c] [-d] windows|xrlinux|android|native [install|docinstall|...]
# 	-c : clean build directory
#	-d : all of targets use debug mode
#	
#	export CMAKE_COMMAND_COMMON=""
#	export CONAN_USER_REQUIRES=""
#	export CONAN_COMMAND_COMMON=""
# 	export BUILD_DIR_SUFFIX=""
#
#	Please install python3(.exe) (latest) and conan 2.0 clang-format and doxygen
#	Make sure add {software}/bin above to windows environment variables.
#	Pass windows (ubuntu sub-system) test
#  

echo "$@"
set -e

# parse options
CLEAN_BUILD_DIR=0
DEBUG_MODE=0
SHARD_LIBRARY="False"

while getopts 'cds' OPT; do
    case $OPT in
        c)
			echo "CLEAN BUILD DIR!!!"
			CLEAN_BUILD_DIR=1
			;;
        d)
			echo "USE DEBUG MODE!!!"
			DEBUG_MODE=1
			;;
        s)
			echo "COMPILE SHARD LIBRARY!!!"
			SHARD_LIBRARY="True"
			;;
        ?)
            echo "compile.sh options error!"
			exit 1
			;;
    esac
done

shift $(($OPTIND - 1))

SYSTEM=$1
SUB_SYSTEM=$1
if [ "${SUB_SYSTEM}" == "android64" ] || [ "${SUB_SYSTEM}" == "android32" ]
then
	SYSTEM="android"
fi

COMMAND=$2

echo "${SYSTEM}"
echo "${SUB_SYSTEM}"
echo "${COMMAND}"

CONAN_BIN=conan
PYTHON_BIN=python3
GRADLE_BIN=./gradlew
# if [ "${SYSTEM}" == "windows" ]
# then
# 	CONAN_BIN=conan.exe
# 	PYTHON_BIN=python3.exe
# fi

OS=`${PYTHON_BIN} -c "import platform; print(platform.system().lower())"`
echo "OS: ${OS}"

PROJECT_FULL_DIR=`pwd`
PROJECT_DIR=.
CONAN_PROFILES=conan_profiles
${PYTHON_BIN} -c "import os; os.symlink(os.path.join('external','project','conan','profiles'), '${CONAN_PROFILES}', target_is_directory=True) if not os.path.exists('${CONAN_PROFILES}') and not os.path.islink('${CONAN_PROFILES}') else None"

CONAN_PROFILE_DIR=${PROJECT_DIR}/${CONAN_PROFILES}

BASE_COMMAND=""

if [ "${SYSTEM}" == "android" ];
then
	if [ "${SUB_SYSTEM}" == "android32" ];
	then
		BUILD_DIR=${PROJECT_DIR}/build_android32
	else
		BUILD_DIR=${PROJECT_DIR}/build_android
	fi
	INSTALL_DIR_SUB=android
	BASE_COMMAND="${BASE_COMMAND} -pr ${CONAN_PROFILE_DIR}/android.profile"
elif [ "${SYSTEM}" == "ios" ] || [ "${SYSTEM}" == "windows" ] || [ "${SYSTEM}" == "macos" ] || [ "${SYSTEM}" == "linux" ] || [ "${SYSTEM}" == "xrlinux" ];
then
	BUILD_DIR=${PROJECT_DIR}/build_${SYSTEM}
	INSTALL_DIR_SUB=${SYSTEM}
	BASE_COMMAND="${BASE_COMMAND} -pr ${CONAN_PROFILE_DIR}/${SYSTEM}.profile"
else
	BUILD_DIR=${PROJECT_DIR}/build_native
	INSTALL_DIR_SUB=native
	BASE_COMMAND="${BASE_COMMAND} -pr ${CONAN_PROFILE_DIR}/${OS}.profile"
fi

if [ ! -z "${BUILD_DIR_SUFFIX}" ]
then
    BUILD_DIR="${BUILD_DIR}_${BUILD_DIR_SUFFIX}"
fi

INSTALL_DIR="${BUILD_DIR}/${INSTALL_DIR_SUB}"

if [ "${CLEAN_BUILD_DIR}" == "1" ]
then 
	rm -rf ${BUILD_DIR}
fi

if [ "${SUB_SYSTEM}" == "android32" ]
then
	BASE_COMMAND="${BASE_COMMAND} -s arch=armv7"
fi

BASE_COMMAND="${BASE_COMMAND} -o shared=${SHARD_LIBRARY} -vvv -c user.build:build_folder=${BUILD_DIR} -c user.build:install_folder_sub=${INSTALL_DIR_SUB} -c user.cmake:command_line=\"${CMAKE_COMMAND_COMMON}\" -c user:requires=\"${CONAN_USER_REQUIRES}\" ${CONAN_COMMAND_COMMON}"

BUILD_COMMAND_RELEASE="${CONAN_BIN} build ${PROJECT_DIR} --update --build=never -s build_type=Release ${BASE_COMMAND}"
BUILD_COMMAND_DEBUG="${CONAN_BIN} build ${PROJECT_DIR} --update --build=never  -s build_type=Debug ${BASE_COMMAND}"
GRAPH_COMMAND="${CONAN_BIN} graph info ${PROJECT_DIR} --update --build=never -s build_type=Release ${BASE_COMMAND}"

INSTALL_COMMAND="${CONAN_BIN} export-pkg ${PROJECT_DIR} ${BASE_COMMAND}"
if [ "${DEBUG_MODE}" == "1" ]
then
	BUILD_COMMAND_RELEASE=${BUILD_COMMAND_DEBUG}
	INSTALL_COMMAND="${INSTALL_COMMAND} -s build_type=Debug"
fi

# JAR related
JAR_BUILD_COMMAND=""
JAR_CLEAN_COMMAND=""
JAR_INSTALL_COMMAND=""
if [[ "${SYSTEM}" == "android" && -f "build.gradle" ]]
then
	JAR_BUILD_COMMAND="${GRADLE_BIN} --info makeJar -PBUILD_DIR=${BUILD_DIR}"
	JAR_CLEAN_COMMAND="${GRADLE_BIN} --info clean -PBUILD_DIR=${BUILD_DIR}"
	JAR_INSTALL_COMMAND="cp -r ${BUILD_DIR}/jar ${INSTALL_DIR}"
fi

echo "BUILD_COMMAND_DEBUG: ${BUILD_COMMAND_DEBUG}"
echo "BUILD_COMMAND_RELEASE: ${BUILD_COMMAND_RELEASE}"
echo "INSTALL_COMMAND: ${INSTALL_COMMAND}"
echo "JAR_BUILD_COMMAND: ${JAR_BUILD_COMMAND}"
echo "JAR_INSTALL_COMMAND: ${JAR_INSTALL_COMMAND}"

runGraph() {
	echo ">>>>>>>>>>runGraph begin"
	eval ${GRAPH_COMMAND}
	echo ">>>>>>>>>runGraph end"
}
runSoBuild() {
	echo ">>>>>>>>>>runSoBuild begin"
	eval ${BUILD_COMMAND_RELEASE}
	echo ">>>>>>>>>runSoBuild end"
}

runJarBuild() {
	echo ">>>>>>>>>>runJarBuild begin"
	eval ${JAR_BUILD_COMMAND}
	echo ">>>>>>>>>runJarBuild end"
}

runDocBuild() {
	eval ${BUILD_COMMAND_RELEASE} -c user.cmake:target_command_line="-DBUILD_DOCS=ON" -c user.cmake:target=doc
}

runAllBuild() {
	echo ">>>>>>>>>>runAllBuild begin"
	eval ${BUILD_COMMAND_RELEASE} -c user.cmake:target_command_line="-DBUILD_DOCS=ON"
	eval ${JAR_BUILD_COMMAND}
	echo ">>>>>>>>>runAllBuild end"
}

runSoInstall() {
	echo ">>>>>>>>>>runSoInstall begin"
	eval ${BUILD_COMMAND_RELEASE}
	eval ${INSTALL_COMMAND}
	echo ">>>>>>>>>>runSoInstall end"
}

runJarInstall() {
	echo ">>>>>>>>>>runJarInstall begin"
	eval ${JAR_BUILD_COMMAND}
	eval ${JAR_INSTALL_COMMAND}
	echo ">>>>>>>>>>runJarInstall end"
}

runAllInstall() {
	echo ">>>>>>>>>>runAllInstall begin"
	eval ${BUILD_COMMAND_RELEASE} -c user.cmake:target_command_line="-DBUILD_DOCS=ON"
	eval ${INSTALL_COMMAND}
	eval ${JAR_BUILD_COMMAND}
	eval ${JAR_INSTALL_COMMAND}
	echo ">>>>>>>>>runAllInstall end"
}

runOnlyInstall() {
	echo ">>>>>>>>>>runOnlyInstall begin"
	eval ${JAR_INSTALL_COMMAND} # conan will copy jar from jar install dir to so install dir, so before so install.
	eval ${INSTALL_COMMAND}
	echo ">>>>>>>>>runOnlyInstall end"
}

runApps() {
	eval ${BUILD_COMMAND_RELEASE} -c user.cmake:target_command_line="-DBUILD_APPS=ON"
}

runAppsInstall() {
	eval ${BUILD_COMMAND_RELEASE} -c user.cmake:target_command_line="-DBUILD_APPS=ON"
	eval ${INSTALL_COMMAND}
}

runTests() {
	eval ${BUILD_COMMAND_RELEASE} -c user.cmake:target_command_line="-DBUILD_TESTS=ON"
	if [ "${DEBUG_MODE}" == "1" ]
	then
		cd ${BUILD_DIR}/Debug
	else
		cd ${BUILD_DIR}/Release
	fi
	ctest -VV
}

runFormat() {
	eval ${BUILD_COMMAND_RELEASE} -c user.cmake:target=format
}


runCoverage() {
	eval ${BUILD_COMMAND_DEBUG} -c user.cmake:target_command_line="-DBUILD_TESTS=ON -DENABLE_COVERAGE=ON" -c user.cmake:target=coverage
}

runTidy() {
	eval ${BUILD_COMMAND_DEBUG} -c user.cmake:target_command_line="-DENABLE_CC_CHECK=ON -DENABLE_CLANG_TIDY=ON" -c user.cmake:target=tidy
}

runASAN() {
	eval ${BUILD_COMMAND_DEBUG} -c user.cmake:target_command_line="-DBUILD_TESTS=ON -DENABLE_CC_CHECK=ON -DENABLE_ASAN=ON"
	cd ${BUILD_DIR}
	ctest -VV
}

runTSAN() {
	eval ${BUILD_COMMAND_DEBUG} -c user.cmake:target_command_line="-DBUILD_TESTS=ON -DENABLE_CC_CHECK=ON -DENABLE_TSAN=ON"
	cd ${BUILD_DIR}
	ctest -VV
}

runMemCheck() {
	eval ${BUILD_COMMAND_DEBUG} -c user.cmake:target_command_line="-DBUILD_TESTS=ON -DENABLE_CC_CHECK=ON -DENABLE_MEMORY_CHECK=ON"
	cd ${BUILD_DIR}
	ctest -T memcheck -VV
}

case "${COMMAND}" in
	"graph")
		runGraph
		;;
	"sobuild")
		runSoBuild
		;;
	"jarbuild")
		runJarBuild
		;;
	"docbuild")
		runDocBuild
		;;
	"allbuild")
		runAllBuild
		;;
	"soinstall")
		runSoInstall
		;;
	"jarinstall")
		runJarInstall
		;;
	"install")
		runSoInstall
		;;
	"allinstall")
		runAllInstall
		;;
	"onlyinstall")
		runOnlyInstall
		;;
	"app")
		runApps
		;;
	"appinstall")
		runAppsInstall
		;;
	"test")
		runTests
		;;
	"format")
		#centos require: scl enable llvm-toolset-7 bash/zsh
		runFormat
		;;
	 "coverage")
	 	runCoverage
	 	;;
	"tidy")
		#centos require: scl enable llvm-toolset-7 bash/zsh
		runTidy
		;;
	"asan")
		#centos require: scl enable devtoolset-4 bash/zsh
		runASAN
		;;
	"tsan")
		#centos require: scl enable devtoolset-4 bash/zsh
		runTSAN
		;;
	"memcheck")
		runMemCheck
		;;
	*)
		runSoBuild
		exit 1
		;;
esac
