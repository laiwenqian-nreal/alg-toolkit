from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, load, replace_in_file, save, copy
from conan.tools.scm import Version
import os

required_conan_version = ">=1.53.0"


class ZlibConan(ConanFile):
    name = "zlib"
    package_type = "library"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://zlib.net"
    license = "Zlib"
    description = ("A Massively Spiffy Yet Delicately Unobtrusive Compression Library "
                   "(Also Free, Not to Mention Unencumbered by Patents)")
    topics = ("zlib", "compression")

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
    }

    @property
    def _is_mingw(self):
        return self.settings.os == "Windows" and self.settings.compiler == "gcc"

    def export_sources(self):
        export_conandata_patches(self)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def layout(self):
        cmake_layout(self, src_folder="src")

    def package_id(self):
        del self.info.settings.build_type
        if self.info.settings.os == "Macos":
            del self.info.settings.arch

    def source(self):
        get(self, **self.conan_data["zlib"][self.version],
            destination=self.source_folder, strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self)

        # MacOS can support both architectures, but Conan doesn't support it yet
        if self.settings.os == "Macos":
            tc.blocks["apple_system"].values["cmake_osx_architectures"] = "arm64;x86_64"

        tc.variables["SKIP_INSTALL_ALL"] = False
        tc.variables["SKIP_INSTALL_LIBRARIES"] = False
        tc.variables["SKIP_INSTALL_HEADERS"] = False
        tc.variables["SKIP_INSTALL_FILES"] = True
        # Correct for misuse of "${CMAKE_INSTALL_PREFIX}/" in CMakeLists.txt
        tc.variables["INSTALL_LIB_DIR"] = "lib"
        tc.variables["INSTALL_INC_DIR"] = "include"

        # fix compiling warning as error
        # tc.variables["CMAKE_C_FLAGS_INIT"] = "${CMAKE_C_FLAGS_INIT} -Wno-error -Wno-implicit-function-declaration -Wno-implicit-fallthrough"
        # if self.settings.compiler in ["apple-clang", "clang"]:
        #     tc.variables["CMAKE_C_FLAGS_INIT"] += " -Wno-unknown-warning-option -Wno-deprecated-non-prototype"

        tc.generate()

    def _patch_sources(self):
        apply_conandata_patches(self)

        is_apple_clang12 = self.settings.compiler == "apple-clang" and Version(self.settings.compiler.version) >= "12.0"
        if not is_apple_clang12:
            for filename in ['zconf.h', 'zconf.h.cmakein', 'zconf.h.in']:
                filepath = os.path.join(self.source_folder, filename)
                replace_in_file(self, filepath,
                                      '#ifdef HAVE_UNISTD_H    '
                                      '/* may be set to #if 1 by ./configure */',
                                      '#if defined(HAVE_UNISTD_H) && (1-HAVE_UNISTD_H-1 != 0)')
                replace_in_file(self, filepath,
                                      '#ifdef HAVE_STDARG_H    '
                                      '/* may be set to #if 1 by ./configure */',
                                      '#if defined(HAVE_STDARG_H) && (1-HAVE_STDARG_H-1 != 0)')

        # using static lib to compile test because of CMAKE_C_VISIBILITY_PRESET is false
        cmakelists_filepath = os.path.join(self.source_folder, 'CMakeLists.txt')
        replace_in_file(self, cmakelists_filepath, 'add_executable(example test/example.c)', 'if(0)\n''add_executable(example test/example.c)')
        replace_in_file(self, cmakelists_filepath, 'target_link_libraries(minigzip zlib)', 'target_link_libraries(minigzip zlib)\n''endif()')
        replace_in_file(self, cmakelists_filepath, 'add_executable(example64 test/example.c)', 'if(0)\n''add_executable(example64 test/example.c)')
        replace_in_file(self, cmakelists_filepath, 'set_target_properties(minigzip64 PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")', 'set_target_properties(minigzip64 PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")\n''endif()')
        # only install static library, because user only use static library
        replace_in_file(self, cmakelists_filepath, 'install(TARGETS zlib zlibstatic', 'install(TARGETS zlibstatic')

    def build(self):
        self._patch_sources()
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def _extract_license(self):
        tmp = load(self, os.path.join(self.source_folder, "zlib.h"))
        license_contents = tmp[2:tmp.find("*/", 1)]
        return license_contents

    def package(self):
        save(self, os.path.join(self.package_folder, "licenses", "LICENSE"), self._extract_license())
        copy(self, pattern="*.h", 
                src=os.path.join(self.source_folder, "contrib", "minizip"), 
                dst=os.path.join(self.package_folder, "include"))
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "ZLIB")
        self.cpp_info.set_property("cmake_target_name", "ZLIB::ZLIB")
        self.cpp_info.set_property("pkg_config_name", "zlib")
        if self.settings.os == "Windows":# and not self._is_mingw:
            libname = "zlib" if self.options.shared else "zlibstatic"
        else:
            libname = "z"
        self.cpp_info.libs = [libname]

        self.cpp_info.names["cmake_find_package"] = "ZLIB"
        self.cpp_info.names["cmake_find_package_multi"] = "ZLIB"


