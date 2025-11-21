from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import CMake
import os


class Project(ConanFile):

    # all project are the same:
    python_requires = "project_base/1.0"
    python_requires_extend = "project_base.ProjectBase"

    def init(self):
        base = self.python_requires["project_base"].module.ProjectBase
        self.settings = base.settings
        self.options.update(base.options, base.default_options)
        self.revision_mode = base.revision_mode

    def requirements(self):
        self.requires(super().override_require("framework/develop"), run=True)
        self.requires("eigen/3.3.7")
        self.requires("grpc/1.72.1")
        self.requires("protobuf/5.27.0")
    
    def build_requirements(self):
        super().build_requirements()
        self.tool_requires("protobuf/5.27.0")
    
    def package_info(self):
        # for find_package(toolkit)
        self.cpp_info.set_property("cmake_file_name", "toolkit")
        self.cpp_info.cxxflags = ["-fno-rtti"]
        self.cpp_info.libs = ["toolkit", "datadump"] 
