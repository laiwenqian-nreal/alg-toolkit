#!/bin/bash

set -e
# in aarch64 ubuntu docker
sudo apt install -y build-essential autoconf bison flex texinfo \
     help2man gawk libtool libtool-bin libtool-doc libncurses5-dev python3-dev \
     python3-distutils git unzip

WORK_DIR=~
INSTALL_DIR=~/x-tools
CT_NG=../crosstool-ng/ct-ng

ulimit -n 2048

# require case-sensitive operation system

# in macos 
# hdiutil create ~/crosstool.dmg -volname "ctng" -size 15g -fs "Case-sensitive APFS"
# hdiutil mount ~/crosstool.dmg

CURRENT_DIR=`pwd`

# compile crosstool-ng
cd $WORK_DIR
git clone http://github.com/crosstool-ng/crosstool-ng
cd crosstool-ng
git checkout crosstool-ng-1.26.0
mv packages/gcc-linaro/7.4-2019.02 packages/gcc-linaro/7.5-2019.12
cp ${CURRENT_DIR}/gcc-linaro-7.5-2019.12-chksum packages/gcc-linaro/7.5-2019.12/chksum
./bootstrap
./configure --enable-local
make

# build aarch64-linux-gnu(linaro) for aarch64-linux 
cd $WORK_DIR
BUILD_DIR=aarch64-linux-gnu
TARGET_NAME=gcc-linaro-7.5.0-2019.12-aarch64_aarch64-linux-gnu
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cp ${CURRENT_DIR}/aarch64-linux-gnu-config .config
# ${CT_NG} ${BUILD_DIR}
# # select PATH/experimental 
# # select c-library/glibc 2.25
# # select c-compile/linaro gcc 7.5.0 
# ${CT_NG} menuconfig
${CT_NG} build

cd $INSTALL_DIR
chmod ug+w -R ${BUILD_DIR}
mv ${BUILD_DIR} ${TARGET_NAME}
tar -czf ${TARGET_NAME}.tar.gz ${TARGET_NAME}


# build x86_64-multilib-linux-gnu for aarch64-linux
cd $WORK_DIR
BUILD_DIR=x86_64-multilib-linux-gnu
TARGET_NAME=gcc-7.5.0-2019.12-aarch64_x86_64-linux-gnu
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cp ${CURRENT_DIR}/x86_64-multilib-linux-gnu-config .config
# ${CT_NG} ${BUILD_DIR}
# # select c-library/glibc 2.25
# # select c-compile/gcc 7.5.0 
# # select c-compiler/multilib-m32,m64   no mx32
# # select c-compiler/Fortran
# # select debug/no gdb
# ${CT_NG} menuconfig
${CT_NG} build

cd $INSTALL_DIR
chmod ug+w -R ${BUILD_DIR}
mv ${BUILD_DIR} ${TARGET_NAME}
tar -czf ${TARGET_NAME}.tar.gz ${TARGET_NAME}

echo "sha256sum:"
sha256sum *.tar.gz
