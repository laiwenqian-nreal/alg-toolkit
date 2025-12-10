from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.scm import Git
import os

class JniWrapConan(ConanFile):
    name = "jniwrap"
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
    }

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.get_safe("shared"):
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def package_id(self):
        del self.info.settings.build_type
        if self.info.settings.os == "Macos":
            del self.info.settings.arch

    def validate(self):
        if self.settings.os != "Android": 
            raise ConanInvalidConfiguration("jniwrap is not supported except android")

    def source(self):
        git = Git(self)
        git.fetch_commit(**self.conan_data[self.name][self.version], commit = self.version)

    def generate(self):
        tc = CMakeToolchain(self)

        # MacOS can support both architectures, but Conan doesn't support it yet
        if self.settings.os == "Macos":
            tc.blocks["apple_system"].values["cmake_osx_architectures"] = "arm64;x86_64"

        tc.generate()
        cmake_deps = CMakeDeps(self)
        cmake_deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "jniwrap")
        self.cpp_info.set_property("cmake_target_name", "jniwrap::jniwrap")
        self.cpp_info.set_property("pkg_config_name", "jniwrap")
        self.cpp_info.libs = ["jniwrap"]
