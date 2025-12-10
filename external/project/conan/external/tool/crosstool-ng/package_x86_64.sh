#!/bin/bash

set -e

if [ -z "$1" ]; then
	echo "usage: $0 SRC_DIR (~/.conan2/p/b/gcc205ff2f3900b6/p/)"
	exit 1
fi

# SRC_DIR is from conan build gcc
# SRC_DIR=~/.conan2/p/b/gcc205ff2f3900b6/p/
SRC_DIR=$1
echo $SRC_DIR
DST_FOLDER=gcc-7.5.0-2019.12-x86_64-pc-linux-gnu

DIR_PARENT_DIR=~/compiler
rm -rf ${DIR_PARENT_DIR}
mkdir -p ${DIR_PARENT_DIR}

DST_DIR=${DIR_PARENT_DIR}/$DST_FOLDER
echo "cp -r $SRC_DIR $DST_DIR"
cp -r $SRC_DIR $DST_DIR

cd $DST_DIR
rm conaninfo.txt conanmanifest.txt

cd ${DIR_PARENT_DIR}
echo "tar -czf $DST_FOLDER.tar.gz $DST_FOLDER"
tar -czf $DST_FOLDER.tar.gz $DST_FOLDER

echo "sha256sum $DST_FOLDER.tar.gz"
sha256sum $DST_FOLDER.tar.gz
