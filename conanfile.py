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


    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "toolkit") # for find_package(toolkit)
        self.cpp_info.cxxflags = ["-fno-rtti"]
        self.cpp_info.libs = ["toolkit"]