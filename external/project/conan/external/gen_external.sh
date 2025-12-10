#!/bin/bash
set -e
Usage="Usage: $0 [macos|linux]"
if [ $# -eq 1 ]; then
    OS=$1
else
    echo $Usage
    exit 1
fi
PYTHON_BIN=python3
# if [ "${OS}" == "windows" ]; then
# 	PYTHON_BIN="python3.exe"
# fi

${PYTHON_BIN} main.py remove local
${PYTHON_BIN} main.py native > log 2>&1

if [ "${OS}" == "macos" ]; then
	${PYTHON_BIN} main.py windows >> log 2>&1
	${PYTHON_BIN} main.py ios >> log 2>&1
fi

if [ "${OS}" == "linux" ]; then
	${PYTHON_BIN} main.py xrlinux >> log 2>&1
	${PYTHON_BIN} main.py android >> log 2>&1 
fi

# conan create project.conan.py
# conan upload project_base -r jfrog-3rdparty
#
# ${PYTHON_BIN} main.py remove jfrog-3rdparty
# ${PYTHON_BIN} main.py upload jfrog-3rdparty

