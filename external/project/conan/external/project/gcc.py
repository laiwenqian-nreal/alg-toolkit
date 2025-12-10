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
    description = (
        "The GNU Compiler Collection includes front ends for C, "
        "C++, Objective-C, Fortran, Ada, Go, and D, as well as "
        "libraries for these languages (libstdc++,...). "
    )
    topics = ("gcc", "gnu", "compiler", "c", "c++")
    homepage = "https://gcc.gnu.org"
    url = "https://github.com/conan-io/conan-center-index"
    license = "GPL-3.0-only"
    settings = "os", "compiler", "arch", "build_type"

    def configure(self):
        if self.settings.compiler in ["clang", "apple-clang"]:
            # Can't remove this from cxxflags with autotools - so get rid of it
            del self.settings.compiler.libcxx

    def package_id(self):
        del self.info.settings.compiler
        del self.info.settings.build_type
        if self.info.settings.os == "Macos":
            del self.info.settings.arch

    def validate_build(self):
        if is_msvc(self):
            raise ConanInvalidConfiguration("GCC can't be built with MSVC")

    def validate(self):
        if self.settings.os == "Windows":
            raise ConanInvalidConfiguration(
                "Windows builds aren't currently supported. Contributions to support this are welcome."
            )
        if self.settings.os == "Macos":
            # FIXME: This recipe should largely support Macos, however the following
            # errors are present when building using the c3i CI:
            # clang: error: unsupported option '-print-multi-os-directory'
            # clang: error: no input files
            raise ConanInvalidConfiguration(
                "Macos builds aren't currently supported. Contributions to support this are welcome."
            )
        if cross_building(self):
            raise ConanInvalidConfiguration(
                "Cross builds are not current supported. Contributions to support this are welcome"
            )

    def layout(self):
        basic_layout(self, src_folder="src")

    def generate(self):
        cmd = "cd " + self.source_folder + " && ./"
        cmd += os.path.join("contrib", "download_prerequisites")
        self.output.info("download cmd : " + cmd)
        self.run(cmd)

    def source(self):
        get(self, **self.conan_data["gcc"][self.version], strip_root=True)

    def build(self):
        # # If building on x86_64, change the default directory name for 64-bit libraries to "lib":
        # replace_in_file(
        #     self,
        #     os.path.join(self.source_folder, "gcc", "config", "i386", "t-linux64"),
        #     "m64=../lib64",
        #     "m64=../lib",
        #     strict=False,
        # )

        # Ensure correct install names when linking against libgcc_s;
        # see discussion in https://github.com/Homebrew/legacy-homebrew/pull/34303
        replace_in_file(
            self,
            os.path.join(self.source_folder, "libgcc", "config", "t-slibgcc-darwin"),
            "@shlib_slibdir@",
            os.path.join(self.package_folder, "lib"),
            strict=False,
        )

        configure_cmd = self.source_folder + "/configure --enable-languages=c,c++,fortran --with-gcc-major-version-only --enable-shared --enable-linker-build-id --without-included-gettext --enable-threads=posix --enable-nls --enable-bootstrap --enable-clocale=gnu --enable-libstdcxx-debug --enable-libstdcxx-time=yes --with-default-libstdcxx-abi=new --enable-gnu-unique-object --disable-vtable-verify --enable-libmpx --enable-plugin --enable-default-pie --with-system-zlib --with-target-system-zlib=auto --enable-multiarch --disable-werror --with-arch-32=i686 --with-abi=m64 --with-multilib-list=m32,m64 --enable-multilib --with-tune=generic --enable-offload-targets=nvptx-none --without-cuda-driver --enable-checking=release --prefix=" + self.package_folder

        self.output.info("configure cmd : " + configure_cmd)
        self.run(configure_cmd)
        self.run("make -j")


    def package(self):
        self.run("make install")

        rmdir(self, os.path.join(self.package_folder, "share"))
        rm(self, "*.la", self.package_folder, recursive=True)
        copy(
            self,
            pattern="COPYING*",
            dst=os.path.join(self.package_folder, "licenses"),
            src=self.source_folder,
            keep_path=False,
        )

    def package_info(self):
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs.append("m")
            self.cpp_info.system_libs.append("rt")
            self.cpp_info.system_libs.append("pthread")
            self.cpp_info.system_libs.append("dl")

        bindir = os.path.join(self.package_folder, "bin")

        cc = os.path.join(bindir, f"gcc")
        self.output.info("Creating CC env var with: " + cc)
        self.buildenv_info.define("CC", cc)

        cxx = os.path.join(bindir, f"g++")
        self.output.info("Creating CXX env var with: " + cxx)
        self.buildenv_info.define("CXX", cxx)

        fc = os.path.join(bindir, f"gfortran")
        self.output.info("Creating FC env var with: " + fc)
        self.buildenv_info.define("FC", fc)

        ar = os.path.join(bindir, f"gcc-ar")
        self.output.info("Creating AR env var with: " + ar)
        self.buildenv_info.define("AR", ar)

        nm = os.path.join(bindir, f"gcc-nm")
        self.output.info("Creating NM env var with: " + nm)
        self.buildenv_info.define("NM", nm)

        ranlib = os.path.join(bindir, f"gcc-ranlib")
        self.output.info("Creating RANLIB env var with: " + ranlib)
        self.buildenv_info.define("RANLIB", ranlib)

        # for ubuntu find lib
        self.buildenv_info.append_path("LIBRARY_PATH", "/usr/lib/x86_64-linux-gnu")
