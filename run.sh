#!/bin/bash
PROJECT_PATH=/home/xlz/work_ws/project/
python3 -c """
import os
os.makedirs('external', exist_ok=True)
try:
	os.symlink('${PROJECT_PATH}', os.path.join('external','project'), target_is_directory=True)
except FileExistsError:
    pass
"""

#export ANDROID_NDK=
#export OBFUSCATE_BIN="/home/xlz/work_ws/nreal_util/framework/build_native/Release/apps/obfuscate"
#bash obfuscate.sh
bash compile.sh $@
