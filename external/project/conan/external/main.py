import os
import sys
import platform
import subprocess

# the sequence is important
# the compiler should be built firstly
# spdlog depend on fmt
# protobuf depend on zlib
project = {
        # "gcc" : "7.5.0",
        "libiconv" : "1.17",
        "sdl" : "2.30.5",
        "libmp3lame" : "3.100",
        "ffmpeg" : "6.1.1",
        "ffmpeg_static" : "6.1.1",
        "fmt" : "9.1.0",
        "stb" : "cci.20230920",
        "fast-cpp-csv-parser" : "cci.20240102",
        "spdlog" : "1.14.1",
        "doctest" : "2.4.11",
        "zlib" : "1.2.13",
        "jsoncpp" : "1.9.5",
        "protobuf" : "3.21.9", 
        "flatbuffers" : "24.12.23", 
        "jniwrap" : "master",
        "spirv-headers" : "1.3.296.0",
        "spirv-tools" : "1.3.296.0",
        "glslang" : "1.3.296.0",
        "vulkan-headers" : "1.3.296.0",
        "vulkan-loader" : "1.3.296.0",
        "freetype" : "2.12.1",
        "perfetto" : "48.1-shared", # require global_conan_cmake_toolchain.cmake export symbols
        "perfetto" : "48.1",
        }
# project = {
#         "eigen" : "3.3.7",
#         }
build_type = ["Release"]
# build_type = ["Debug", "Release"]
glslang_project_list = ["spirv-headers", "spirv-tools", "glslang"]
vulkan_project_list = ["vulkan-headers", "vulkan-loader"]

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python {0} [native|android|ios|windows|xrlinux|armlinux|remove|upload]".format(sys.argv[0]))
        exit(-1)

    if sys.argv[1] == "remove" or sys.argv[1] == "upload":
        for k, v in project.items():
            cmd = 'conan {conan_cmd} -v -c {project}'.format(
                    conan_cmd=sys.argv[1],
                    project=k, 
                    )
            if len(sys.argv) >= 3 and sys.argv[2] != "local":
                cmd += " -r {0}".format(sys.argv[2])
            print(cmd)
            if 0 != subprocess.call(cmd, shell=True):
                pass
                # exit(-1)
        exit(0)

    if sys.argv[1] == "native":
        current_os = platform.system().lower()
        arch = ""
    elif sys.argv[1] == "android32":
        current_os = sys.argv[1][:-2]
        arch = "-s arch=armv7"
    else:
        current_os = sys.argv[1]
        arch = ""
    print("os:" + current_os)

    current_file_path = os.path.dirname(os.path.realpath(__file__))
    print(current_file_path)

    os.environ["GLOBAL_CONAN_CMAKE_TOOLCHAIN_FILE"] = os.path.join(
            current_file_path, "global_conan_cmake_toolchain.cmake")
    for k, v in project.items():
        if k in glslang_project_list and (current_os == "linux" or current_os == "armlinux" or current_os == "xrlinux"):
            continue
        if k in vulkan_project_list and (current_os != "windows" and current_os != "darwin"):
            continue
        if k == "jniwrap" and current_os != "android":
            continue
        if k == "libmp3lame" and current_os != "xrlinux":
            continue
        if k == "sdl" and ( current_os == "linux" or current_os == "armlinux" or current_os == "xrlinux" ):
            continue
        if (k == "gcc") and (current_os != "linux" and current_os != "armlinux"):
            continue
        for b in build_type:
            if (k == "gcc") and b == "Debug":
                continue
            cmd = 'conan create -vvv\
            project/{project}.py --version={version} \
            -pr=../profiles/external.profile -pr=../profiles/{current_os}.profile \
            -s build_type={build_type} {arch}'.format(
                current_os=current_os,
                project=k, 
                version=v,
                build_type=b,
                arch=arch
                )
            print("Start to compile {0} {1} {2} in {3}".format(k, v, b, current_os))
            print(cmd)
            if 0 != subprocess.call(cmd, shell=True):
                exit(-1)

