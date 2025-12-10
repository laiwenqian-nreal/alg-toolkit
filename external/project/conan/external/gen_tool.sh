#!/bin/bash
Usage="Usage: $0 [macos|linux]"
if [ $# -eq 1 ]; then
    OS=$1
else
    echo $Usage
    exit 1
fi
conan remove cmake -c
conan remove linaro -c
conan remove gcc -c
conan remove mingw -c
conan remove android-ndk -c
conan remove ios-cmake -c
conan remove jwasm -c

set -e
conan create -vvv tool/cmake.py --version=3.25.3 -pr=tool_profiles/darwin.profile
conan create -vvv tool/cmake.py --version=3.25.3 -pr=tool_profiles/linux.profile
conan create -vvv tool/cmake.py --version=3.25.3 -pr=tool_profiles/armlinux.profile

conan create -vvv tool/linaro.py --version=7.5.0 -pr=tool_profiles/linux.profile
conan create -vvv tool/linaro.py --version=7.5.0 -pr=tool_profiles/armlinux.profile

conan create -vvv tool/gcc.py --version=7.5.0 -pr=tool_profiles/linux.profile
conan create -vvv tool/gcc.py --version=7.5.0 -pr=tool_profiles/armlinux.profile

conan create -vvv tool/mingw.py --version=20220323 -pr=tool_profiles/darwin.profile
conan create -vvv tool/mingw.py --version=20220323 -pr=tool_profiles/linux.profile

conan create -vvv tool/ios-cmake.py --version=4.4.0 -pr=tool_profiles/darwin.profile
conan create -vvv tool/ios-cmake.py --version=4.5.0 -pr=tool_profiles/darwin.profile

conan create -vvv tool/android-ndk.py --version=r25c -pr=tool_profiles/darwin.profile
conan create -vvv tool/android-ndk.py --version=r25c -pr=tool_profiles/linux.profile
conan create -vvv tool/android-ndk.py --version=r27c -pr=tool_profiles/darwin.profile
conan create -vvv tool/android-ndk.py --version=r27c -pr=tool_profiles/linux.profile

if [ "${OS}" == "macos" ]; then
	conan create -vvv tool/jwasm.py --version=jwasm-2.13 -pr=tool_profiles/darwin.profile
fi

if [ "${OS}" == "linux" ]; then
	conan create -vvv tool/jwasm.py --version=jwasm-2.13 -pr=tool_profiles/linux.profile
	conan create -vvv tool/jwasm.py --version=jwasm-2.13 -pr=tool_profiles/armlinux.profile
fi

# conan upload cmake -r jfrog-3rdparty
# conan upload linaro -r jfrog-3rdparty
# conan upload gcc -r jfrog-3rdparty
# conan upload mingw -r jfrog-3rdparty
# conan upload android-ndk -r jfrog-3rdparty
# conan upload ios-cmake -r jfrog-3rdparty
# conan upload jwasm -r jfrog-3rdparty
