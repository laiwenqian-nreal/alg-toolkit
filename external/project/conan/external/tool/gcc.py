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

required_conan_version = ">=1.55.0"


class GccConan(ConanFile):
    name = "gcc"
    settings = "os", "arch"

    def package_id(self):
        if self.info.settings.os == "Macos":
            del self.info.settings.arch

    def build(self):
        arch = str(self.settings.arch) if self.settings.os != "Macos" else "universal"
        get(self, **self.conan_data["gcc"][self.version][str(self.settings.os)][arch], 
                destination=self.source_folder, strip_root=True)

    def package(self):
        self.output.info("source_folder: {}, package_folder: {}".format(self.source_folder, self.package_folder))
        copy(self, "*", src=self.source_folder, dst=self.package_folder)

        rmdir(self, os.path.join(self.package_folder, "share"))
        rm(self, "*.txt", self.package_folder)

    def package_info(self):
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs.append("m")
            self.cpp_info.system_libs.append("rt")
            self.cpp_info.system_libs.append("pthread")
            self.cpp_info.system_libs.append("dl")

        bindir = os.path.join(self.package_folder, "bin")

        bin_prefix = ""
        if self.settings.arch == "armv8":
            bin_prefix = "x86_64-multilib-linux-gnu-"

        cc = os.path.join(bindir, f"{bin_prefix}gcc")
        self.output.info("Creating CC env var with: " + cc)
        self.buildenv_info.define("CC", cc)

        cxx = os.path.join(bindir, f"{bin_prefix}g++")
        self.output.info("Creating CXX env var with: " + cxx)
        self.buildenv_info.define("CXX", cxx)

        fc = os.path.join(bindir, f"{bin_prefix}gfortran")
        self.output.info("Creating FC env var with: " + fc)
        self.buildenv_info.define("FC", fc)

        ar = os.path.join(bindir, f"{bin_prefix}gcc-ar")
        self.output.info("Creating AR env var with: " + ar)
        self.buildenv_info.define("AR", ar)

        nm = os.path.join(bindir, f"{bin_prefix}gcc-nm")
        self.output.info("Creating NM env var with: " + nm)
        self.buildenv_info.define("NM", nm)

        ranlib = os.path.join(bindir, f"{bin_prefix}gcc-ranlib")
        self.output.info("Creating RANLIB env var with: " + ranlib)
        self.buildenv_info.define("RANLIB", ranlib)

        # for ubuntu find lib
        self.buildenv_info.append_path("LIBRARY_PATH", "/usr/lib/aarch64-linux-gnu:/usr/lib/x86_64-linux-gnu")
