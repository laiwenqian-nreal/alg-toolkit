from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy
from conan.tools.scm import Git
import os

class ProjectConanFile(ConanFile):
    name = "project_base"
    version = "1.0"
    package_type = "python-require"

class ProjectBase:
    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    # Other configurations
    revision_mode = "scm"

    def set_name(self):
        git = Git(self)
        url = git.run("config --get remote.origin.url")
        base_name = os.path.basename(url).lower()
        self.name = os.path.splitext(base_name)[0] if base_name.endswith('.git') else base_name

    def set_version(self):
        git = Git(self)
        version = git.run("branch --show-current")
        # if self.version is already defined from CLI --version arg, it will not use git branch
        self.version = self.version or version.replace("/", "-")

    def override_require(self, default_require):
        override_requires = self.conf.get("user:requires", [])
        for r in override_requires:
            if r.split("/")[0] == default_require.split("/")[0]:
                return r
        return default_require

    def build_requirements(self):
        pass;

    def layout(self):
        # The root of the project is one level above
        # self.folders.root = ".."
        build_folder = self.conf.get("user.build:build_folder", "build")
        self.output.info("user.build:build_folder: %s" % build_folder)
        cmake_layout(self, build_folder=build_folder)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)

        tc.generate()

    def pre_build(self):
        build_target = self.conf.get("user.cmake:target", None)
        self.output.info("user.cmake:target: %s" % build_target)

        command_line = self.conf.get("user.cmake:command_line", "")
        self.output.info("user.cmake:command_line: %s" % command_line)
        target_command_line = self.conf.get("user.cmake:target_command_line", "")
        self.output.info("user.cmake:target_command_line: %s" % target_command_line)

        variables = {}
        line = command_line + " " + target_command_line
        self.output.info("all cmake defines: %s" % line)
        for v in line.split():
            if v.startswith("-D"):
                s = v[2:].split("=")
                if len(s[0]) > 0 and len(s[1]) > 0:
                    variables[s[0]] = s[1]

        defines = self.conf.get("user.cmake:defines", default=[], check_type=list)
        self.output.info("user.cmake:defines: %s" % defines)
        for v in defines:
            s = v.split("=")
            if len(s[0]) > 0 and len(s[1]) > 0:
                variables[s[0]] = s[1]

        variables["CMAKE_BUILD_TYPE"] = self.settings.build_type;

        return variables, build_target

    def post_build(self, variables, build_target):
        cmake = CMake(self)
        cmake.configure(variables=variables)
        cmake.build(target=build_target, build_type=None)

    def build(self):
        variables, build_target = self.pre_build()
        self.post_build(variables, build_target)

    def package(self):
        build_folder = self.conf.get("user.build:build_folder", "build")
        install_folder_sub = self.conf.get("user.build:install_folder_sub", "install")
        copy(self, pattern="*.jar", src=os.path.join(self.source_folder, build_folder, install_folder_sub), dst=os.path.join(self.package_folder, "jar"))
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        pass

