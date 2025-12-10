#!/bin/bash

#	Usage:
#	1. the parameters is the same as compile.sh.
#	2. this file should be also linked to the same dir/path as compile.sh
#	
#	Pay attention:
#	the docker will mount the parent directory of the directory this file is linked to.
#	the parent directory should be also include the project.git, 
# 
#	the parent directory:
#   workplace ( docker mount dir )
#		|
#		|	---- project (project.git)
#		|	---- xxxx (eg. framework)
#					|
#					| ---- docker_compile.sh (link to external/project/docker_compile.sh)
#					| ---- compile.sh
#					| ---- external 
#								| ---- project (link to ../../project, this must be relative path) 
#
#	export DOCKER_ARCH="amd64" (arm64 or amd64, default is arm64) --> docker容器的架构
#	export DOCKER_STANDALONE="false" (true or false, default is true) --> 同时编译多个项目时，需要设置为false
#	export DOCKER_COMPILE="none" (none or sdk or rust, default is sdk) --> 是否编译 none/sdk/rust

set -e
ROOT_DIR=`dirname "$(pwd)"`
CURRUNT_DIR=`basename "$(pwd)"`
echo "mount dir:$ROOT_DIR"
echo "current dir:$CURRUNT_DIR"

NAME=sdk-compile
if [ "$DOCKER_COMPILE" == "rust" ]; then
	NAME=sdk-compile-rust
fi
if [ "$DOCKER_ARCH" == "amd64" ]; then
	NAME=${NAME}-amd64
else
	NAME=${NAME}-arm64
fi
DOCKER_WORKPLACE=workplace
COMPILE_PARAMETERS="$@"
DOCKER_DIR=external/project/docker

cd ${DOCKER_DIR}
mkdir -p .conan2 .ccache
cd -

if [ "$DOCKER_STANDALONE" != "false" ]; then
	set +e 
	docker stop ${NAME}
	docker rm ${NAME}
	docker rmi ${NAME}
	set -e
fi

DOCKER_BUILD_COMMAND_COMMON=""
DOCKER_RUN_COMMAND_COMMON=""
if [ "$DOCKER_ARCH" == "amd64" ]; then
	DOCKER_BUILD_COMMAND_COMMON="${DOCKER_BUILD_COMMAND_COMMON} --platform=linux/amd64"
	DOCKER_RUN_COMMAND_COMMON="${DOCKER_RUN_COMMAND_COMMON} --platform=linux/amd64"
else
	DOCKER_BUILD_COMMAND_COMMON="${DOCKER_BUILD_COMMAND_COMMON} --platform=linux/arm64"
	DOCKER_RUN_COMMAND_COMMON="${DOCKER_RUN_COMMAND_COMMON} --platform=linux/arm64"
fi

echo "DOCKER_COMPILE:$DOCKER_COMPILE"
if [ "$DOCKER_COMPILE" == "rust" ]; then
docker build \
	${DOCKER_BUILD_COMMAND_COMMON} \
	--build-arg UID=$(id -u) \
	--build-arg GID=$(id -g) \
	--build-arg SUPPORT_RUST=true \
	-t ${NAME} \
	-f ${DOCKER_DIR}/sdk.dockerfile \
	${DOCKER_DIR}
else
docker build \
	${DOCKER_BUILD_COMMAND_COMMON} \
	--build-arg UID=$(id -u) \
	--build-arg GID=$(id -g) \
	-t ${NAME} \
	-f ${DOCKER_DIR}/sdk.dockerfile \
	${DOCKER_DIR}
fi

if [ "$DOCKER_COMPILE" = "none" ] || [ "$DOCKER_COMPILE" = "rust" ]; then
docker run \
		--name ${NAME} \
		${DOCKER_RUN_COMMAND_COMMON} \
		-it \
		-v ${ROOT_DIR}:/home/dev/${DOCKER_WORKPLACE} \
		-v ${ROOT_DIR}/${CURRUNT_DIR}/${DOCKER_DIR}/.conan2:/home/dev/.conan2 \
		-v ${ROOT_DIR}/${CURRUNT_DIR}/${DOCKER_DIR}/.ccache:/home/dev/.ccache \
		-v /etc/timezone:/etc/timezone:ro \
		-v /etc/localtime:/etc/localtime:ro \
		${NAME}
else
docker run \
		--name ${NAME} \
		${DOCKER_RUN_COMMAND_COMMON} \
		-it \
		-v ${ROOT_DIR}:/home/dev/${DOCKER_WORKPLACE} \
		-v ${ROOT_DIR}/${CURRUNT_DIR}/${DOCKER_DIR}/.conan2:/home/dev/.conan2 \
		-v ${ROOT_DIR}/${CURRUNT_DIR}/${DOCKER_DIR}/.ccache:/home/dev/.ccache \
		-v /etc/timezone:/etc/timezone:ro \
		-v /etc/localtime:/etc/localtime:ro \
		${NAME} /bin/bash -c "export CONAN_USER_REQUIRES=\"${CONAN_USER_REQUIRES}\" && export CMAKE_COMMAND_COMMON=\"${CMAKE_COMMAND_COMMON}\" && export CONAN_COMMAND_COMMON=\"${CONAN_COMMAND_COMMON}\" && cd ${DOCKER_WORKPLACE}/${CURRUNT_DIR} && ./compile.sh ${COMPILE_PARAMETERS}"
fi
