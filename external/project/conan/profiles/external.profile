[conf]
tools.build:jobs={{os.cpu_count()}}
tools.build:skip_test=True
tools.cmake.cmaketoolchain:user_toolchain+=$ENV{GLOBAL_CONAN_CMAKE_TOOLCHAIN_FILE}
