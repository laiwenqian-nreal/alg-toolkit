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


class MingwConan(ConanFile):
    name = "mingw"
    settings = "os", "arch"

    def package_id(self):
        if self.info.settings.os == "Macos":
            del self.info.settings.arch

    def build(self):
        arch = str(self.settings.arch) if self.settings.os != "Macos" else "universal"
        get(self, **self.conan_data["mingw"][self.version][str(self.settings.os)][arch], 
                destination=self.source_folder, strip_root=True)

    def package(self):
        self.output.info("source_folder: {}, package_folder: {}".format(self.source_folder, self.package_folder))
        copy(self, "*", src=os.path.join(self.source_folder,"bin"), dst=os.path.join(self.package_folder,"bin"))
        copy(self, "*", src=os.path.join(self.source_folder,"lib"), dst=os.path.join(self.package_folder,"lib"))
        copy(self, "*", src=os.path.join(self.source_folder,"include"), dst=os.path.join(self.package_folder,"include"))
        copy(self, "*", src=os.path.join(self.source_folder,"x86_64-w64-mingw32"), dst=os.path.join(self.package_folder,"x86_64-w64-mingw32"))
        copy(self, "*", src=os.path.join(self.source_folder,"generic-w64-mingw32"), dst=os.path.join(self.package_folder,"generic-w64-mingw32"))

    def package_info(self):
        self.conf_info.define("tools.cmake.cmaketoolchain:system_name", "Windows")
        bindir = os.path.join(self.package_folder, "bin")

        cc = os.path.join(bindir, f"x86_64-w64-mingw32-clang")
        self.output.info("Creating CC env var with: " + cc)
        self.buildenv_info.define_path("CC", cc)

        cxx = os.path.join(bindir, f"x86_64-w64-mingw32-clang++")
        self.output.info("Creating CXX env var with: " + cxx)
        self.buildenv_info.define_path("CXX", cxx)

        ar = os.path.join(bindir, f"x86_64-w64-mingw32-ar")
        self.output.info("Creating AR env var with: " + ar)
        self.buildenv_info.define_path("AR", ar)

        nm = os.path.join(bindir, f"x86_64-w64-mingw32-nm")
        self.output.info("Creating NM env var with: " + nm)
        self.buildenv_info.define_path("NM", nm)

        ranlib = os.path.join(bindir, f"x86_64-w64-mingw32-ranlib")
        self.output.info("Creating RANLIB env var with: " + ranlib)
        self.buildenv_info.define_path("RANLIB", ranlib)

        strip = os.path.join(bindir, f"x86_64-w64-mingw32-strip")
        self.output.info("Creating STRIP env var with: " + strip)
        self.buildenv_info.define_path("STRIP", strip)

        ld = os.path.join(bindir, f"x86_64-w64-mingw32-ld")
        self.output.info("Creating LD env var with: " + ld)
        self.buildenv_info.define_path("LD", ld)

        readelf = os.path.join(bindir, f"x86_64-w64-mingw32-readelf")
        self.output.info("Creating READELF env var with: " + readelf)
        self.buildenv_info.define_path("READELF", readelf)

        objcopy = os.path.join(bindir, f"x86_64-w64-mingw32-objcopy")
        self.output.info("Creating OBJCOPY env var with: " + objcopy)
        self.buildenv_info.define_path("OBJCOPY", objcopy)

        objdump = os.path.join(bindir, f"x86_64-w64-mingw32-objdump")
        self.output.info("Creating OBJDUMP env var with: " + objdump)
        self.buildenv_info.define_path("OBJDUMP", objdump)
