from conan import ConanFile
from conan.tools.gnu import Autotools, AutotoolsToolchain
from conan.errors import ConanInvalidConfiguration
from conan.tools.layout import basic_layout
from conan.tools.apple import XCRun
from conan.tools.files import copy, get, replace_in_file, rmdir, rm
from conan.tools.build import cross_building
from conan.tools.env import VirtualBuildEnv
from conan.tools.microsoft import is_msvc
import os

required_conan_version = ">=2.0.0"


class OdbConan(ConanFile):
    name = "odb"
    settings = "os", "arch"

    def package_id(self):
        if self.info.settings.os == "Macos":
            del self.info.settings.arch

    def build(self):
        arch = str(self.settings.arch) if self.settings.os != "Macos" else "universal"
        get(self, **self.conan_data["odb"][self.version][str(self.settings.os)][arch], 
                destination=self.source_folder, strip_root=True)

    def package(self):
        self.output.info("source_folder: {}, package_folder: {}".format(self.source_folder, self.package_folder))
        copy(self, "*", src=self.source_folder, dst=self.package_folder)

        rmdir(self, os.path.join(self.package_folder, "share"))
        rm(self, "*.txt", self.package_folder)

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "odb")
        self.cpp_info.libs = ["odb"]
