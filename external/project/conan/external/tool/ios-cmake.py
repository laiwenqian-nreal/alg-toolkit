from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import is_apple_os
from conan.tools.files import get, save, copy

import os


class IosCMakeConan(ConanFile):
    name = "ios-cmake"
    license = "BSD-3-Clause"
    settings = "os" , "arch"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://github.com/leetal/ios-cmake"
    options = {
        "enable_bitcode": [True, False],
        "enable_arc": [True, False],
        "enable_visibility": [True, False],
        "enable_strict_try_compile": [True, False],
        "toolchain_target": ["auto", "OS", "OS64", "OS64COMBINED",
                       "SIMULATOR", "SIMULATOR64", "SIMULATORARM64",
                       "TVOS", "TVOSCOMBINED",
                       "SIMULATOR_TVOS", "WATCHOS",
                       "WATCHOSCOMBINED", "SIMULATOR_WATCHOS",
                       "MAC", "MAC_ARM64", "MAC_CATALYST", "MAC_CATALYST_ARM64"]
    }
    default_options = {
        "enable_bitcode": False,
        "enable_arc": True,
        "enable_visibility": False,
        "enable_strict_try_compile": False,
        "toolchain_target": "auto",
    }
    description = "ios Cmake toolchain to (cross) compile macOS/iOS/watchOS/tvOS"
    topics = "conan", "apple", "ios", "cmake", "toolchain", "ios", "tvos", "watchos"
    exports_sources =  "ios-cmake-wrapper"
    wrapper_file =  "ios-cmake-wrapper"

    def configure(self):
        if not is_apple_os(self):
            raise ConanInvalidConfiguration("This package only supports Apple operating systems")

    def package_id(self):
        self.info.clear()

    def source(self):
        get(self, **self.conan_data["ios-cmake"][self.version], destination=self.source_folder, strip_root=True)

    def build(self):
        pass

    @staticmethod
    def _chmod_plus_x(filename):
        if os.name == 'posix':
            os.chmod(filename, os.stat(filename).st_mode | 0o111)

    def package(self):
        self.output.info("copy {} from {} to {}".format(self.wrapper_file, os.path.join(self.source_folder, os.pardir), os.path.join(self.package_folder, "bin", self.wrapper_file)))
        copy(self, self.wrapper_file, src=os.path.join(self.source_folder), dst=os.path.join(self.package_folder, "bin"))
        self._chmod_plus_x(os.path.join(self.package_folder, "bin", self.wrapper_file))
        copy(self, "ios.toolchain.cmake",
                    src=self.source_folder,
                    dst=os.path.join(self.package_folder, "lib", "cmake", "ios-cmake"))

    def _guess_toolchain_target(self, os, arch):
        if os == "iOS":
            if arch in ["armv8", "armv8.3"]:
                return "OS64"
            else:
                raise ConanInvalidConfiguration("Not supported iOS arch")
        elif os == "Macos":
            return "MAC_UNIVERSAL"
        else:
            raise ConanInvalidConfiguration("Not iOS or Macos target")

    def package_info(self):

        if not hasattr(self, "settings_target"):
            return
        if self.settings_target is None:
            return

        target_os = str(self.settings_target.os)
        arch_flag = self.settings_target.arch
        target_version = self.settings_target.os.version

        if self.options.toolchain_target == "auto":
            toolchain_target = self._guess_toolchain_target(target_os, arch_flag)
        else:
            toolchain_target = self.options.toolchain_target

        if arch_flag == "armv8":
            arch_flag = "arm64"
        elif arch_flag == "armv8.3":
            arch_flag = "arm64e"

        cmake_options = "-DENABLE_BITCODE={} -DENABLE_ARC={} -DENABLE_VISIBILITY={} -DENABLE_STRICT_TRY_COMPILE={}".format(
            self.options.enable_bitcode,
            self.options.enable_arc,
            self.options.enable_visibility,
            self.options.enable_strict_try_compile
        )
        # Note that this, as long as we specify (overwrite) the ARCHS, PLATFORM has just limited effect,
        # but PLATFORM need to be set in the profile so it makes sense, see ios-cmake docs for more info
        cmake_flags = "-DPLATFORM={} -DDEPLOYMENT_TARGET={} -DARCHS={} {}".format(
            toolchain_target, target_version, arch_flag, cmake_options
        )
        if self.settings_target.os == "Macos":
            cmake_flags = "-DPLATFORM={} -DDEPLOYMENT_TARGET={} {}".format(
                    toolchain_target, target_version, cmake_options
                )

        self.buildenv_info.define("CONAN_USER_CMAKE_FLAGS", cmake_flags)

        cmake_wrapper = os.path.join(self.package_folder, "bin", self.wrapper_file)
        self.output.info("Setting CONAN_CMAKE_PROGRAM to: {}".format(cmake_wrapper))
        self.conf_info.define("tools.cmake:cmake_program", cmake_wrapper)

        self.output.info("Setting toolchain options to: {}".format(cmake_flags))
        tool_chain = os.path.join(self.package_folder,
                                    "lib",
                                    "cmake",
                                    "ios-cmake",
                                    "ios.toolchain.cmake")
        self.conf_info.define("tools.cmake.cmaketoolchain:user_toolchain", [tool_chain])
        self.conf_info.define("tools.cmake.cmaketoolchain:generator", "Xcode")
