[settings]
arch=armv8
build_type=Release
compiler=gcc
compiler.cppstd=17
compiler.libcxx=libstdc++11
compiler.version=7.5
os=Linux
[conf]
tools.cmake.cmaketoolchain:system_name=Linux
tools.cmake.cmaketoolchain:system_processor=aarch64
# for conanfile
user.os:distro=Xrlinux
# for cmake
user.cmake:defines=['Xrlinux=ON']
# for compiler
tools.build:defines=['Xrlinux']
[tool_requires]
cmake/3.25.3
linaro/7.5.0
