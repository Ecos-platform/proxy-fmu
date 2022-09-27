from conans import ConanFile, CMake, tools
from os import path


class ProxyFmuConan(ConanFile):
    name = "proxyfmu"
    author = "NTNU Aalesund"
    license = "MIT"
    exports = "version.txt"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake", "cmake_find_package"
    requires = (
        "cli11/2.2.0",
        "thrift/0.17.0",
        "fmilibcpp/0.2.2@ais/stable"
    )

    def set_version(self):
        self.version = tools.load(path.join(self.recipe_folder, "version.txt")).strip()

    def configure_cmake(self):
        cmake = CMake(self)
        cmake.configure()
        return cmake

    def build(self):
        cmake = self.configure_cmake()
        cmake.build()

    def package(self):
        cmake = self.configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["proxyfmu-client"]
